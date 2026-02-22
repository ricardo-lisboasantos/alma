#include "test_util.h"
#include <random>

namespace {
void fill_identity(std::vector<double>& M, int n) {
    M.assign(n * n, 0.0);
    for (int i = 0; i < n; ++i) M[i * n + i] = 1.0;
}

void fill_zeros(std::vector<double>& M, int n) {
    M.assign(n * n, 0.0);
}

void fill_ones(std::vector<double>& M, int n) {
    M.assign(n * n, 1.0);
}

void fill_random(std::vector<double>& M, int n) {
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> dist(-10.0, 10.0);
    M.resize(n * n);
    for (auto& v : M) v = dist(gen);
}

void fill_row_vector(std::vector<double>& M, int n) {
    M.resize(n * n, 0.0);
    for (int j = 0; j < n; ++j) M[j] = j + 1.0;
}

void fill_col_vector(std::vector<double>& M, int n) {
    M.resize(n * n, 0.0);
    for (int i = 0; i < n; ++i) M[i * n] = i + 1.0;
}

void fill_triangular_lower(std::vector<double>& M, int n) {
    M.resize(n * n, 0.0);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j <= i; ++j) {
            M[i * n + j] = (i - j + 1);
        }
    }
}

void fill_triangular_upper(std::vector<double>& M, int n) {
    M.resize(n * n, 0.0);
    for (int i = 0; i < n; ++i) {
        for (int j = i; j < n; ++j) {
            M[i * n + j] = (j - i + 1);
        }
    }
}

void fill_permutation(std::vector<double>& M, int n) {
    M.assign(n * n, 0.0);
    for (int i = 0; i < n; ++i) M[i * n + (n - 1 - i)] = 1.0;
}

void fill_symmetric(std::vector<double>& M, int n) {
    M.resize(n * n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            M[i * n + j] = M[j * n + i] = (i == j) ? 2.0 : 1.0;
        }
    }
}

void fill_diagonal(std::vector<double>& M, int n) {
    M.assign(n * n, 0.0);
    for (int i = 0; i < n; ++i) M[i * n + i] = 3.14;
}

bool test_identity_x_identity() { std::vector<double> A, B; fill_identity(A, 64); fill_identity(B, 64); return test_matrix(64, 16, A, B); }
bool test_identity_x_random() { std::vector<double> A, B; fill_identity(A, 64); fill_random(B, 64); return test_matrix(64, 16, A, B); }
bool test_random_x_identity() { std::vector<double> A, B; fill_random(A, 64); fill_identity(B, 64); return test_matrix(64, 16, A, B); }
bool test_zeros_x_random() { std::vector<double> A, B; fill_zeros(A, 64); fill_random(B, 64); return test_matrix(64, 16, A, B); }
bool test_random_x_zeros() { std::vector<double> A, B; fill_random(A, 64); fill_zeros(B, 64); return test_matrix(64, 16, A, B); }
bool test_ones() { std::vector<double> A, B; fill_ones(A, 64); fill_ones(B, 64); return test_matrix(64, 16, A, B); }
bool test_row_col_vector() { std::vector<double> A, B; fill_row_vector(A, 16); fill_col_vector(B, 16); return test_matrix(16, 4, A, B); }
bool test_lower_triangular() { std::vector<double> A, B; fill_triangular_lower(A, 32); fill_triangular_lower(B, 32); return test_matrix(32, 8, A, B); }
bool test_upper_triangular() { std::vector<double> A, B; fill_triangular_upper(A, 32); fill_triangular_upper(B, 32); return test_matrix(32, 8, A, B); }
bool test_permutation() { std::vector<double> A, B; fill_permutation(A, 32); fill_permutation(B, 32); return test_matrix(32, 8, A, B); }
bool test_symmetric() { std::vector<double> A, B; fill_symmetric(A, 32); fill_symmetric(B, 32); return test_matrix(32, 8, A, B); }
bool test_diagonal() { std::vector<double> A, B; fill_diagonal(A, 64); fill_diagonal(B, 64); return test_matrix(64, 16, A, B); }
}

REGISTER_TEST("Identity x Identity", test_identity_x_identity);
REGISTER_TEST("Identity x Random", test_identity_x_random);
REGISTER_TEST("Random x Identity", test_random_x_identity);
REGISTER_TEST("Zeros x Random", test_zeros_x_random);
REGISTER_TEST("Random x Zeros", test_random_x_zeros);
REGISTER_TEST("All ones", test_ones);
REGISTER_TEST("Row x Col vector", test_row_col_vector);
REGISTER_TEST("Lower triangular", test_lower_triangular);
REGISTER_TEST("Upper triangular", test_upper_triangular);
REGISTER_TEST("Permutation", test_permutation);
REGISTER_TEST("Symmetric", test_symmetric);
REGISTER_TEST("Diagonal", test_diagonal);
