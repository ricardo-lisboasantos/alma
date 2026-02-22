#include "test_util.h"

bool test_csv() {
    std::vector<double> A, B;
    int n = 0;
    if (!load_csv("bench/data/matrix1.csv", A, n)) return false;
    if (!load_csv("bench/data/matrix2.csv", B, n)) return false;
    return test_matrix(n, 128, A, B);
}

REGISTER_TEST("CSV 512x512", test_csv);
