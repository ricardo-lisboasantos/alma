#pragma once

#include <cstddef>

enum class AlmaError {
    Success = 0,
    NullPointer,
    InvalidDimension,
    InvalidBlockSize,
    DimensionMismatch,
    SingularMatrix,
    NotImplemented
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

enum class NormType { One, Two, Inf, Frobenius };

double alma_norm(double* A, int m, int n, NormType type);

AlmaError alma_inverse(double* A, double* invA, int n);

double alma_determinant(double* A, int n);

struct SVDResult {
    double* U;
    double* S;
    double* VT;
    int m, n, k;
};

AlmaError alma_svd(double* A, SVDResult& result, int m, int n);
void alma_svd_free(SVDResult& result);

struct LUResult {
    double* LU;
    int* pivots;
    int n;
};

AlmaError alma_lu(double* A, LUResult& result, int n);
void alma_lu_free(LUResult& result);

struct QRResult {
    double* Q;
    double* R;
    int m, n;
};

AlmaError alma_qr(double* A, QRResult& result, int m, int n);
void alma_qr_free(QRResult& result);

AlmaError alma_solve(double* A, double* B, double* X, int n, int nrhs);
AlmaError alma_solve_lu(const LUResult& lu, double* B, double* X, int n, int nrhs);

AlmaError alma_transpose(double* A, double* AT, int m, int n);
AlmaError alma_add(double* A, double* B, double* C, int m, int n, double alpha, double beta);
AlmaError alma_scale(double* A, int m, int n, double scalar);
