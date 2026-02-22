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

bool test_1x1() { std::vector<double> A, B; fill_pattern(A, 1); fill_pattern(B, 1); return test_matrix(1, 1, A, B); }
bool test_2x2_large_block() { std::vector<double> A, B; fill_pattern(A, 2); fill_pattern(B, 2); return test_matrix(2, 4, A, B); }
bool test_8x8_block1() { std::vector<double> A, B; fill_pattern(A, 8); fill_pattern(B, 8); return test_matrix(8, 1, A, B); }
bool test_8x8_block_eq() { std::vector<double> A, B; fill_pattern(A, 8); fill_pattern(B, 8); return test_matrix(8, 8, A, B); }
bool test_128x128_small_block() { std::vector<double> A, B; fill_pattern(A, 128); fill_pattern(B, 128); return test_matrix(128, 16, A, B); }
}

REGISTER_TEST("1x1 block=1", test_1x1);
REGISTER_TEST("2x2 block=4", test_2x2_large_block);
REGISTER_TEST("8x8 block=1", test_8x8_block1);
REGISTER_TEST("8x8 block=8", test_8x8_block_eq);
REGISTER_TEST("128x128 block=16", test_128x128_small_block);
