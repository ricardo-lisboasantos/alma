#include "alma.h"
#include <cmath>
#include <cblas.h>
#include <omp.h>
#include <cstdlib>

static constexpr double LOWRANK_THRESHOLD = 1e-3;

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

static void classify_all_blocks(double* A, BlockMeta* blockMetaA, int n, int blockSize) {
    int numBlocks = n / blockSize;

    #pragma omp parallel for schedule(static)
    for (int bi = 0; bi < numBlocks; ++bi) {
        for (int bj = 0; bj < numBlocks; ++bj) {
            double* data = &A[(bi * blockSize) * n + bj * blockSize];
            compute_block_meta(data, blockMetaA[bi * numBlocks + bj], blockSize, blockSize, n);
        }
    }
}

void alma_multiply(double* A, double* B, double* C,
                   int n, int blockSize) {
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

    classify_all_blocks(A, metaA, n, blockSize);
    classify_all_blocks(B, metaB, n, blockSize);

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
