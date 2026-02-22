#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include "../src/alma.h"

static void ref_mul(const double* A, const double* B, double* C, int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double s = 0.0;
            for (int k = 0; k < n; ++k) s += A[i*n + k] * B[k*n + j];
            C[i*n + j] = s;
        }
    }
}

bool load_csv(const std::string& path, std::vector<double>& M, int& n) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    std::vector<std::vector<double>> rows;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::vector<double> row;
        double val;
        while (ss >> val) {
            row.push_back(val);
            if (ss.peek() == ',') ss.ignore();
        }
        if (!row.empty()) rows.push_back(std::move(row));
    }
    
    if (rows.empty()) return false;
    
    n = rows.size();
    M.resize(n * n);
    for (int i = 0; i < n; ++i) {
        if ((int)rows[i].size() != n) return false;
        for (int j = 0; j < n; ++j) {
            M[i * n + j] = rows[i][j];
        }
    }
    return true;
}

bool test_size(int n, int block) {
    std::vector<double> A(n*n), B(n*n), C(n*n, 0.0), R(n*n, 0.0);
    
    for (int i = 0; i < n*n; ++i) {
        A[i] = (i % 7) * 0.1;
        B[i] = (i % 5) * 0.2;
    }
    
    ref_mul(A.data(), B.data(), R.data(), n);
    alma_multiply(A.data(), B.data(), C.data(), n, block);
    
    double eps = 1e-9;
    for (int i = 0; i < n*n; ++i) {
        if (std::fabs(R[i] - C[i]) > eps) {
            std::cerr << "Mismatch at index " << i << ": ref=" << R[i] << " got=" << C[i] << "\n";
            return false;
        }
    }
    return true;
}

bool test_csv(const std::string& path_a, const std::string& path_b, int block) {
    std::vector<double> A, B;
    int n = 0, n_a = 0, n_b = 0;
    
    if (!load_csv(path_a, A, n_a)) {
        std::cerr << "Failed to load " << path_a << "\n";
        return false;
    }
    if (!load_csv(path_b, B, n_b)) {
        std::cerr << "Failed to load " << path_b << "\n";
        return false;
    }
    if (n_a != n_b) {
        std::cerr << "Matrix dimension mismatch\n";
        return false;
    }
    n = n_a;
    
    std::vector<double> C(n*n, 0.0), R(n*n, 0.0);
    ref_mul(A.data(), B.data(), R.data(), n);
    alma_multiply(A.data(), B.data(), C.data(), n, block);
    
    double eps = 1e-9;
    for (int i = 0; i < n*n; ++i) {
        if (std::fabs(R[i] - C[i]) > eps) {
            std::cerr << "CSV test mismatch at index " << i << ": ref=" << R[i] << " got=" << C[i] << "\n";
            return false;
        }
    }
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;
    
    std::cout << "Running alma tests...\n\n";
    
    std::cout << "Test: Small matrix (4x4, block=2)... ";
    if (test_size(4, 2)) {
        std::cout << "PASSED\n";
        passed++;
    } else {
        std::cout << "FAILED\n";
        failed++;
    }
    
    std::cout << "Test: Medium matrix (64x64, block=16)... ";
    if (test_size(64, 16)) {
        std::cout << "PASSED\n";
        passed++;
    } else {
        std::cout << "FAILED\n";
        failed++;
    }
    
    std::cout << "Test: Medium matrix (64x64, block=32)... ";
    if (test_size(64, 32)) {
        std::cout << "PASSED\n";
        passed++;
    } else {
        std::cout << "FAILED\n";
        failed++;
    }
    
    std::cout << "Test: Large matrix (256x256, block=64)... ";
    if (test_size(256, 64)) {
        std::cout << "PASSED\n";
        passed++;
    } else {
        std::cout << "FAILED\n";
        failed++;
    }
    
    std::cout << "Test: Large matrix (256x256, block=128)... ";
    if (test_size(256, 128)) {
        std::cout << "PASSED\n";
        passed++;
    } else {
        std::cout << "FAILED\n";
        failed++;
    }
    
    std::cout << "Test: Block larger than size (2x2, block=4)... ";
    if (test_size(2, 4)) {
        std::cout << "PASSED\n";
        passed++;
    } else {
        std::cout << "FAILED\n";
        failed++;
    }
    
    std::cout << "Test: Identity matrix (64x64)... ";
    {
        int n = 64;
        int block = 16;
        std::vector<double> A(n*n, 0.0), B(n*n, 0.0), C(n*n, 0.0), R(n*n, 0.0);
        
        for (int i = 0; i < n; ++i) {
            A[i*n + i] = 1.0;
            B[i*n + i] = 1.0;
        }
        
        ref_mul(A.data(), B.data(), R.data(), n);
        alma_multiply(A.data(), B.data(), C.data(), n, block);
        
        double eps = 1e-9;
        bool ok = true;
        for (int i = 0; i < n*n; ++i) {
            if (std::fabs(R[i] - C[i]) > eps) {
                ok = false;
                break;
            }
        }
        
        if (ok) {
            std::cout << "PASSED\n";
            passed++;
        } else {
            std::cout << "FAILED\n";
            failed++;
        }
    }
    
    std::cout << "Test: Zero matrix (64x64)... ";
    {
        int n = 64;
        int block = 16;
        std::vector<double> A(n*n, 0.0), B(n*n, 1.0), C(n*n, 0.0), R(n*n, 0.0);
        
        ref_mul(A.data(), B.data(), R.data(), n);
        alma_multiply(A.data(), B.data(), C.data(), n, block);
        
        double eps = 1e-9;
        bool ok = true;
        for (int i = 0; i < n*n; ++i) {
            if (std::fabs(R[i] - C[i]) > eps) {
                ok = false;
                break;
            }
        }
        
        if (ok) {
            std::cout << "PASSED\n";
            passed++;
        } else {
            std::cout << "FAILED\n";
            failed++;
        }
    }
    
    std::cout << "Test: CSV loading (512x512)... ";
    if (test_csv("bench/data/matrix1.csv", "bench/data/matrix2.csv", 128)) {
        std::cout << "PASSED\n";
        passed++;
    } else {
        std::cout << "FAILED\n";
        failed++;
    }
    
    std::cout << "\n====================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    
    return failed > 0 ? 1 : 0;
}
