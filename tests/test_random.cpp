#include "test_util.h"
#include <random>

namespace {
void fill_random(std::vector<double>& M, int n, unsigned seed) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dist(-10.0, 10.0);
    M.resize(n * n);
    for (auto& v : M) v = dist(gen);
}

bool test_32x32() { std::vector<double> A, B; fill_random(A, 32, 42); fill_random(B, 32, 123); return test_matrix(32, 8, A, B); }
bool test_64x64() { std::vector<double> A, B; fill_random(A, 64, 42); fill_random(B, 64, 123); return test_matrix(64, 16, A, B); }
bool test_128x128() { std::vector<double> A, B; fill_random(A, 128, 42); fill_random(B, 128, 123); return test_matrix(128, 32, A, B); }
}

REGISTER_TEST("32x32 random", test_32x32);
REGISTER_TEST("64x64 random", test_64x64);
REGISTER_TEST("128x128 random", test_128x128);
