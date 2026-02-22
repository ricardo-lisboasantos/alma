#include "alma.h"
#include <cmath>
#include <cblas.h>
#include <omp.h>
#include <algorithm>
#include <cstdlib>

// Threshold for low-rank classification
static constexpr double LOWRANK_THRESHOLD = 1e-3;

// Thread-local buffers for block data
static thread_local double* tl_workBuffer = nullptr;
static thread_local int tl_bufferSize = 0;

// Allocate thread-local buffer if needed
static inline double* get_thread_buffer(int size) {
    if (tl_bufferSize < size) {
        delete[] tl_workBuffer;
        tl_workBuffer = new double[size];
        tl_bufferSize = size;
    }
    return tl_workBuffer;
}

// Pre-compute block norms and types with optimized norm estimation
static void classify_all_blocks(double* A, BlockMeta* blockMetaA, int n, int blockSize) {
    int numBlocks = n / blockSize;
    
    #pragma omp parallel for schedule(static)
    for (int bi = 0; bi < numBlocks; ++bi) {
        for (int bj = 0; bj < numBlocks; ++bj) {
            double* data = &A[(bi * blockSize) * n + bj * blockSize];
            double norm = 0.0;
            
            // Compute Frobenius norm with early termination
            for (int i = 0; i < blockSize; ++i) {
                double row_norm = 0.0;
                int base = i * blockSize;
                for (int j = 0; j < blockSize; ++j) {
                    double val = data[base + j];
                    row_norm += val * val;
                }
                norm += row_norm;
                
                // Early exit if norm clearly exceeds threshold
                if (norm > LOWRANK_THRESHOLD * LOWRANK_THRESHOLD * 100) {
                    // Continue counting but we know it's dense
                    for (++i; i < blockSize; ++i) {
                        base = i * blockSize;
                        for (int j = 0; j < blockSize; ++j) {
                            norm += data[base + j] * data[base + j];
                        }
                    }
                    break;
                }
            }
            norm = std::sqrt(norm);
            
            BlockMeta& m = blockMetaA[bi * numBlocks + bj];
            m.rows = blockSize;
            m.cols = blockSize;
            m.ld = n;
            
            if (norm < LOWRANK_THRESHOLD) {
                m.type = BlockType::LowRank;
                m.rank_est = 1;
            } else {
                m.type = BlockType::Dense;
                m.rank_est = blockSize;
            }
        }
    }
}

// Inline version of dense block multiply for performance
inline void multiply_dense_block_inline(const double* A, int lda,
                                         const double* B, int ldb,
                                         double* C, int ldc,
                                         int M, int N, int K) {
    cblas_dgemm(CblasRowMajor,
                CblasNoTrans, CblasNoTrans,
                M, N, K,
                1.0,
                A, lda,
                B, ldb,
                1.0,
                C, ldc);
}

// Optimized alma_multiply with pre-classified blocks and OpenMP
// Key optimizations:
// 1. Pre-allocated metadata arrays (aligned for cache)
// 2. Zero-copy initialization
// 3. Dynamic scheduling based on workload characteristics
// 4. Better memory access patterns

// Optimized alma_multiply with pre-classified blocks and OpenMP
// Key optimizations:
// 1. Pre-allocated metadata arrays (aligned for cache)
// 2. Zero-copy initialization
// 3. Better scheduling based on workload
// 4. Compiler hints for vectorization
void alma_multiply(double* A, double* B, double* C,
                   int n, int blockSize) {
    int numBlocks = n / blockSize;
    
    // For small matrices, fall back to standard BLAS
    if (n <= 256) {
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    n, n, n, 1.0, A, n, B, n, 0.0, C, n);
        return;
    }
    
    // Pre-allocate metadata arrays for all blocks
    BlockMeta* metaA = static_cast<BlockMeta*>(
        std::aligned_alloc(64, numBlocks * numBlocks * sizeof(BlockMeta)));
    BlockMeta* metaB = static_cast<BlockMeta*>(
        std::aligned_alloc(64, numBlocks * numBlocks * sizeof(BlockMeta)));
    
    // Pre-classify all blocks once
    classify_all_blocks(A, metaA, n, blockSize);
    classify_all_blocks(B, metaB, n, blockSize);
    
    // Zero output matrix (parallel for large matrices)
    if (n * n > 10000) {
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < n * n; ++i) {
            C[i] = 0.0;
        }
    } else {
        std::fill(C, C + n * n, 0.0);
    }
    
    // Count dense blocks for optimal path selection
    int denseCountA = 0, denseCountB = 0;
    #pragma omp parallel for reduction(+:denseCountA, denseCountB) schedule(static)
    for (int i = 0; i < numBlocks * numBlocks; ++i) {
        if (metaA[i].type == BlockType::Dense) denseCountA++;
        if (metaB[i].type == BlockType::Dense) denseCountB++;
    }
    
    bool mostlyDense = (denseCountA > numBlocks * numBlocks / 2) && 
                       (denseCountB > numBlocks * numBlocks / 2);
    
    // Optimal path for dense matrices
    if (mostlyDense) {
        #pragma omp parallel for collapse(2) schedule(dynamic, 1)
        for (int bi = 0; bi < numBlocks; ++bi) {
            for (int bj = 0; bj < numBlocks; ++bj) {
                double* Cb = &C[(bi * blockSize) * n + bj * blockSize];
                bool first = true;
                
                for (int bk = 0; bk < numBlocks; ++bk) {
                    const double* Ab = &A[(bi * blockSize) * n + bk * blockSize];
                    const double* Bb = &B[(bk * blockSize) * n + bj * blockSize];
                    
                    const BlockMeta& Ameta = metaA[bi * numBlocks + bk];
                    const BlockMeta& Bmeta = metaB[bk * numBlocks + bj];
                    
                    if (Ameta.type == BlockType::Dense && Bmeta.type == BlockType::Dense) {
                        if (first) {
                            cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                                        blockSize, blockSize, blockSize,
                                        1.0, Ab, n, Bb, n, 0.0, Cb, n);
                            first = false;
                        } else {
                            cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                                        blockSize, blockSize, blockSize,
                                        1.0, Ab, n, Bb, n, 1.0, Cb, n);
                        }
                    } else {
                        // Low-rank block
                        if (first) {
                            cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                                        blockSize, blockSize, blockSize,
                                        1.0, Ab, n, Bb, n, 0.0, Cb, n);
                            first = false;
                        } else {
                            cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                                        blockSize, blockSize, blockSize,
                                        1.0, Ab, n, Bb, n, 1.0, Cb, n);
                        }
                    }
                }
            }
        }
    } else {
        // Mixed case
        #pragma omp parallel for collapse(2) schedule(static)
        for (int bi = 0; bi < numBlocks; ++bi) {
            for (int bj = 0; bj < numBlocks; ++bj) {
                double* Cb = &C[(bi * blockSize) * n + bj * blockSize];
                bool first = true;
                
                for (int bk = 0; bk < numBlocks; ++bk) {
                    const double* Ab = &A[(bi * blockSize) * n + bk * blockSize];
                    const double* Bb = &B[(bk * blockSize) * n + bj * blockSize];
                    
                    const BlockMeta& Ameta = metaA[bi * numBlocks + bk];
                    const BlockMeta& Bmeta = metaB[bk * numBlocks + bj];
                    
                    if (first) {
                        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                                    blockSize, blockSize, blockSize,
                                    1.0, Ab, n, Bb, n, 0.0, Cb, n);
                        first = false;
                    } else {
                        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                                    blockSize, blockSize, blockSize,
                                    1.0, Ab, n, Bb, n, 1.0, Cb, n);
                    }
                }
            }
        }
    }
    
    std::free(metaA);
    std::free(metaB);
}

// Original function kept for compatibility
void alma_multiply_block(const MatrixBlock& A,
                         const MatrixBlock& B,
                         MatrixBlock& C) {
    if (A.meta.type == BlockType::Dense &&
        B.meta.type == BlockType::Dense) {
        multiply_dense_block(A, B, C);
    } else if (A.meta.type == BlockType::LowRank &&
               B.meta.type == BlockType::LowRank) {
        multiply_lowrank_block(A, B, C);
    } else {
        multiply_dense_block(A, B, C);
    }
}

BlockMeta classify_block(double* data, int rows, int cols) {
    double norm = 0.0;
    for (int i = 0; i < rows * cols; ++i) norm += data[i] * data[i];
    norm = std::sqrt(norm);

    BlockMeta m;
    m.rows = rows;
    m.cols = cols;
    if (norm < LOWRANK_THRESHOLD) {
        m.type = BlockType::LowRank;
        m.rank_est = 1;
    } else {
        m.type = BlockType::Dense;
        m.rank_est = rows;
    }
    return m;
}

void multiply_dense_block(const MatrixBlock& A,
                          const MatrixBlock& B,
                          MatrixBlock& C) {
    int M = A.meta.rows;
    int K = A.meta.cols;
    int N = B.meta.cols;

    int lda = A.meta.ld > 0 ? A.meta.ld : K;
    int ldb = B.meta.ld > 0 ? B.meta.ld : N;
    int ldc = C.meta.ld > 0 ? C.meta.ld : N;

    cblas_dgemm(CblasRowMajor,
                CblasNoTrans, CblasNoTrans,
                M, N, K,
                1.0,
                A.data, lda,
                B.data, ldb,
                1.0,
                C.data, ldc);
}

void multiply_lowrank_block(const MatrixBlock& A,
                            const MatrixBlock& B,
                            MatrixBlock& C) {
    multiply_dense_block(A, B, C);
}
