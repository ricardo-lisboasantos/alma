#include "test_util.h"

namespace {
void fill_pattern(std::vector<double>& M, int n) {
    M.resize(n * n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            M[i * n + j] = (i + j + 1);
        }
    }
}

bool test_4x4() { std::vector<double> A, B; fill_pattern(A, 4); fill_pattern(B, 4); return test_matrix(4, 2, A, B); }
bool test_8x8() { std::vector<double> A, B; fill_pattern(A, 8); fill_pattern(B, 8); return test_matrix(8, 4, A, B); }
bool test_16x16() { std::vector<double> A, B; fill_pattern(A, 16); fill_pattern(B, 16); return test_matrix(16, 8, A, B); }
bool test_32x32() { std::vector<double> A, B; fill_pattern(A, 32); fill_pattern(B, 32); return test_matrix(32, 16, A, B); }
}

REGISTER_TEST("4x4 block=2", test_4x4);
REGISTER_TEST("8x8 block=4", test_8x8);
REGISTER_TEST("16x16 block=8", test_16x16);
REGISTER_TEST("32x32 block=16", test_32x32);
