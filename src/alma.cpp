#include "alma.h"
#include <cmath>
#include <cblas.h>
#include <omp.h>
#include <cstdlib>
#include <algorithm>
#include <sys/sysctl.h>
#if defined(__linux__)
#include <fstream>
#endif

static constexpr double LOWRANK_THRESHOLD = 1e-3;
static constexpr int POWER_ITERATIONS = 3;

static size_t get_l3_cache_size() {
#if defined(__APPLE__)
    int64_t l3size = 0;
    size_t len = sizeof(l3size);
    sysctlbyname("hw.l3cachesize", &l3size, &len, NULL, 0);
    return static_cast<size_t>(l3size);
#elif defined(__linux__)
    std::ifstream caches("/sys/devices/system/cpu/cpu0/cache/index3/size");
    std::string line;
    if (std::getline(caches, line)) {
        size_t cache = std::stoull(line.substr(0, line.size() - 1));
        return (line.back() == 'K') ? cache * 1024 : cache * 1024 * 1024;
    }
#endif
    return 8UL * 1024 * 1024;
}

static int get_cache_optimal_block_size() {
    size_t cache = get_l3_cache_size();
    return static_cast<int>(std::sqrt(cache / (2.0 * sizeof(double))));
}

static void compute_block_meta(double* data, BlockMeta& m, int rows, int cols, int ld) {
    double norm = 0.0;
    for (int i = 0; i < rows * cols; ++i) norm += data[i] * data[i];
    norm = std::sqrt(norm);

    m.rows = rows;
    m.cols = cols;
    m.ld = ld;
    m.type = (norm < LOWRANK_THRESHOLD) ? BlockType::LowRank : BlockType::Dense;
    m.rank_est = (m.type == BlockType::LowRank) ? 1 : rows;
}

static double matrix_norm(const double* A, int m, int n) {
    double norm = 0.0;
    for (int i = 0; i < m * n; ++i) norm += A[i] * A[i];
    return std::sqrt(norm);
}

static int estimate_rank_svd(const double* data, int rows, int cols, double threshold) {
    if (rows <= 1 || cols <= 1) return (rows * cols > 0) ? 1 : 0;
    
    int m = std::min(rows, cols);
    int n = std::max(rows, cols);
    bool transposed = (cols > rows);
    
    if (transposed) {
        int tmp = m; m = n; n = tmp;
    }
    
    double full_norm = 0.0;
    for (int i = 0; i < rows * cols; ++i) full_norm += data[i] * data[i];
    full_norm = std::sqrt(full_norm);
    
    if (full_norm < LOWRANK_THRESHOLD) return 0;
    
    double trace = 0.0;
    int stride = transposed ? cols : rows;
    for (int i = 0; i < std::min(rows, cols); ++i) {
        trace += data[i * stride + i];
    }
    double diag_norm = std::sqrt(trace * trace);
    
    if (diag_norm > 0.5 * full_norm) {
        double rank_ratio = diag_norm / full_norm;
        if (rank_ratio > 0.9) return 1;
        if (rank_ratio > 0.5) return 2;
        if (rank_ratio > 0.25) return 4;
        if (rank_ratio > 0.1) return m / 4;
    }
    
    return m;
}

static void classify_all_blocks(double* A, BlockMeta* blockMetaA, int n, int blockSize, bool use_svd) {
    int numBlocks = n / blockSize;

    #pragma omp parallel for schedule(static)
    for (int bi = 0; bi < numBlocks; ++bi) {
        for (int bj = 0; bj < numBlocks; ++bj) {
            double* data = &A[(bi * blockSize) * n + bj * blockSize];
            BlockMeta& m = blockMetaA[bi * numBlocks + bj];
            
            if (use_svd) {
                m.rows = blockSize;
                m.cols = blockSize;
                m.ld = n;
                m.rank_est = estimate_rank_svd(data, blockSize, blockSize, LOWRANK_THRESHOLD);
                m.type = (m.rank_est < blockSize / 2) ? BlockType::LowRank : BlockType::Dense;
            } else {
                compute_block_meta(data, m, blockSize, blockSize, n);
            }
        }
    }
}

void alma_multiply_full(double* A, double* B, double* C,
                        int n, int blockSize, bool use_svd);

void alma_multiply(double* A, double* B, double* C,
                   int n, int blockSize) {
    alma_multiply_full(A, B, C, n, blockSize, true);
}

void alma_multiply_full(double* A, double* B, double* C,
                        int n, int blockSize, bool use_svd) {
    int numBlocks = n / blockSize;

    if (n <= 256) {
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    n, n, n, 1.0, A, n, B, n, 0.0, C, n);
        return;
    }

    BlockMeta* metaA = static_cast<BlockMeta*>(
        aligned_alloc(64, numBlocks * numBlocks * sizeof(BlockMeta)));
    BlockMeta* metaB = static_cast<BlockMeta*>(
        aligned_alloc(64, numBlocks * numBlocks * sizeof(BlockMeta)));

    classify_all_blocks(A, metaA, n, blockSize, use_svd);
    classify_all_blocks(B, metaB, n, blockSize, use_svd);

    if (n * n > 10000) {
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < n * n; ++i) {
            C[i] = 0.0;
        }
    } else {
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < n * n; ++i) {
            C[i] = 0.0;
        }
    }

    #pragma omp parallel for collapse(2) schedule(dynamic, 1)
    for (int bi = 0; bi < numBlocks; ++bi) {
        for (int bj = 0; bj < numBlocks; ++bj) {
            double* Cb = &C[(bi * blockSize) * n + bj * blockSize];
            bool first = true;

            for (int bk = 0; bk < numBlocks; ++bk) {
                const double* Ab = &A[(bi * blockSize) * n + bk * blockSize];
                const double* Bb = &B[(bk * blockSize) * n + bj * blockSize];

                int rankA = metaA[bi * numBlocks + bk].rank_est;
                int rankB = metaB[bk * numBlocks + bj].rank_est;
                
                double alpha = first ? 0.0 : 1.0;
                cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                            blockSize, blockSize, blockSize,
                            1.0, Ab, n, Bb, n, alpha, Cb, n);
                first = false;
            }
        }
    }

    free(metaA);
    free(metaB);
}

void alma_multiply_auto(double* A, double* B, double* C, int n) {
    int blockSize = get_cache_optimal_block_size();
    alma_multiply_full(A, B, C, n, blockSize, true);
}

int alma_get_optimal_block_size() {
    return get_cache_optimal_block_size();
}

// Original function kept for compatibility
void alma_multiply_block(const MatrixBlock& A,
                         const MatrixBlock& B,
                         MatrixBlock& C) {
    multiply_dense_block(A, B, C);
}

BlockMeta classify_block(double* data, int rows, int cols) {
    BlockMeta m;
    compute_block_meta(data, m, rows, cols, cols);
    return m;
}

void multiply_dense_block(const MatrixBlock& A,
                          const MatrixBlock& B,
                          MatrixBlock& C) {
    cblas_dgemm(CblasRowMajor,
                CblasNoTrans, CblasNoTrans,
                A.meta.rows, B.meta.cols, A.meta.cols,
                1.0,
                A.data, A.meta.cols,
                B.data, B.meta.cols,
                1.0,
                C.data, C.meta.cols);
}

void multiply_lowrank_block(const MatrixBlock& A,
                           const MatrixBlock& B,
                           MatrixBlock& C) {
    multiply_dense_block(A, B, C);
}
