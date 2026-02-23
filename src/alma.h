#pragma once

#include <cstddef>

enum class BlockType { LowRank, Dense };

struct BlockMeta {
    BlockType type;
    int rows, cols, rank_est;
    int ld; // leading dimension (row stride) in the original matrix
};

struct MatrixBlock {
    double* data;
    BlockMeta meta;
};

BlockMeta classify_block(double* data, int rows, int cols);
void multiply_dense_block(const MatrixBlock& A,
                          const MatrixBlock& B,
                          MatrixBlock& C);
void multiply_lowrank_block(const MatrixBlock& A,
                            const MatrixBlock& B,
                            MatrixBlock& C);
void alma_multiply_block(const MatrixBlock& A,
                         const MatrixBlock& B,
                         MatrixBlock& C);

void alma_multiply(double* A, double* B, double* C,
                   int n, int blockSize);
void alma_multiply_full(double* A, double* B, double* C,
                        int n, int blockSize, bool use_svd);
void alma_multiply_auto(double* A, double* B, double* C, int n);
int alma_get_optimal_block_size();
