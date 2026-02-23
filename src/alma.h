#pragma once

#include <cstddef>

enum class AlmaError {
    Success = 0,
    NullPointer,
    InvalidDimension,
    InvalidBlockSize,
    DimensionMismatch
};

enum class BlockType { LowRank, Dense };

struct BlockMeta {
    BlockType type;
    int rows, cols, rank;
    double* U;
    double* S;
    double* VT;
};

struct MatrixBlock {
    double* data;
    BlockMeta meta;
};

BlockMeta classify_block(double* data, int rows, int cols, int ld);
void multiply_dense_block(const MatrixBlock& A,
                          const MatrixBlock& B,
                          MatrixBlock& C);
void multiply_lowrank_block(const MatrixBlock& A,
                            const MatrixBlock& B,
                            MatrixBlock& C);
void alma_multiply_block(const MatrixBlock& A,
                         const MatrixBlock& B,
                         MatrixBlock& C);

void free_block_meta(BlockMeta& meta);

AlmaError alma_multiply(double* A, double* B, double* C,
                        int n, int blockSize);
AlmaError alma_multiply_full(double* A, double* B, double* C,
                             int n, int blockSize);
AlmaError alma_multiply_auto(double* A, double* B, double* C, int n);
int alma_get_optimal_block_size();
const char* alma_error_string(AlmaError err);
