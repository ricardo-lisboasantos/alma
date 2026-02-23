#include "test_util.h"
#include <fstream>

bool test_csv() {
    std::ifstream f1("bench/data/matrix1.csv");
    std::ifstream f2("bench/data/matrix2.csv");
    if (!f1.is_open() || !f2.is_open()) {
        return true;
    }
    
    std::vector<double> A, B;
    int n = 0;
    if (!load_csv("bench/data/matrix1.csv", A, n)) return false;
    if (!load_csv("bench/data/matrix2.csv", B, n)) return false;
    return test_matrix(n, 128, A, B);
}

REGISTER_TEST("CSV 512x512", test_csv);
