#include "test_util.h"
#include <random>

namespace {
void fill_pattern(std::vector<double>& M, int n) {
    M.resize(n * n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            M[i * n + j] = (i + j + 1);
        }
    }
}

void fill_random(std::vector<double>& M, int n) {
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> dist(-10.0, 10.0);
    M.resize(n * n);
    for (auto& v : M) v = dist(gen);
}

bool test_256x64() { std::vector<double> A, B; fill_pattern(A, 256); fill_pattern(B, 256); return test_matrix(256, 64, A, B); }
bool test_256x128() { std::vector<double> A, B; fill_pattern(A, 256); fill_pattern(B, 256); return test_matrix(256, 128, A, B); }
bool test_256_random() { std::vector<double> A, B; fill_random(A, 256); fill_random(B, 256); return test_matrix(256, 64, A, B); }
}

REGISTER_TEST("256x256 block=64", test_256x64);
REGISTER_TEST("256x256 block=128", test_256x128);
REGISTER_TEST("256x256 random", test_256_random);
