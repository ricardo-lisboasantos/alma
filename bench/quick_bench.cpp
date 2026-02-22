#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <cblas.h>
#include <random>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <functional>
#include "../src/alma.h"

bool load_csv(const std::string& path, std::vector<double>& M, int& n) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << path << "\n";
        return false;
    }
    
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
    
    if (rows.empty()) {
        std::cerr << "Error: Empty CSV file " << path << "\n";
        return false;
    }
    
    n = rows.size();
    M.resize(n * n);
    for (int i = 0; i < n; ++i) {
        if ((int)rows[i].size() != n) {
            std::cerr << "Error: Matrix " << path << " is not square or has inconsistent rows\n";
            return false;
        }
        for (int j = 0; j < n; ++j) {
            M[i * n + j] = rows[i][j];
        }
    }
    return true;
}

using highres_clock = std::chrono::high_resolution_clock;
using duration_ms = std::chrono::duration<double, std::milli>;

// Quick benchmark for a single configuration
double time_kernel(std::function<void()> f, int repeats, bool warmup = true) {
    if (warmup) {
        for (int i = 0; i < 2; ++i) f();
    }

    auto t0 = highres_clock::now();
    for (int i = 0; i < repeats; ++i) f();
    auto t1 = highres_clock::now();
    
    return std::chrono::duration_cast<duration_ms>(t1 - t0).count() / repeats;
}

void random_fill(std::vector<double>& M, std::mt19937& gen) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (size_t i = 0; i < M.size(); ++i) {
        M[i] = dist(gen);
    }
}

void openblas_mul(const double* A, const double* B, double* C, int n) {
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                n, n, n,
                1.0, A, n, B, n, 0.0, C, n);
}

double gflops(int n, double time_ms) {
    return 2.0 * n * n * n / (time_ms * 1e6);
}

int main(int argc, char** argv) {
    int n = 1024;
    int block = 128;
    int repeats = 3;
    std::string matrix_a_path, matrix_b_path;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-a" && i + 1 < argc) {
            matrix_a_path = argv[++i];
        } else if (arg == "-B" && i + 1 < argc) {
            matrix_b_path = argv[++i];
        } else if (arg == "-s" && i + 1 < argc) {
            n = atoi(argv[++i]);
        } else if (arg == "-r" && i + 1 < argc) {
            repeats = atoi(argv[++i]);
        } else if (i == 1) {
            n = atoi(argv[i]);
        } else if (i == 2) {
            block = atoi(argv[i]);
        } else if (i == 3) {
            repeats = atoi(argv[i]);
        }
    }
    
    std::vector<double> A, B;
    if (!matrix_a_path.empty() || !matrix_b_path.empty()) {
        int n_a = 0, n_b = 0;
        if (!matrix_a_path.empty()) {
            if (!load_csv(matrix_a_path, A, n_a)) return 1;
            n = n_a;
        }
        if (!matrix_b_path.empty()) {
            if (!load_csv(matrix_b_path, B, n_b)) return 1;
            if (!matrix_a_path.empty() && n_a != n_b) {
                std::cerr << "Error: Matrix dimensions mismatch\n";
                return 1;
            }
            n = n_b;
        }
    }
    
    std::mt19937 gen(42);
    std::vector<double> C_alma(n*n), C_openblas(n*n);
    
    if (A.empty()) {
        A.resize(n * n);
        random_fill(A, gen);
    }
    if (B.empty()) {
        B.resize(n * n);
        random_fill(B, gen);
    }
    
    std::cout << "Matrix: " << n << "x" << n << ", Block: " << block << "\n";
    std::cout << "========================================\n";
    
    // Time OpenBLAS
    auto openblas_func = [&]() {
        std::fill(C_openblas.begin(), C_openblas.end(), 0.0);
        openblas_mul(A.data(), B.data(), C_openblas.data(), n);
    };
    double t_openblas = time_kernel(openblas_func, repeats);
    
    // Time alma
    auto alma_func = [&]() {
        std::fill(C_alma.begin(), C_alma.end(), 0.0);
        alma_multiply(A.data(), B.data(), C_alma.data(), n, block);
    };
    double t_alma = time_kernel(alma_func, repeats);
    
    // Verify
    double maxdiff = 0.0;
    for (int i = 0; i < n*n; ++i) {
        maxdiff = std::max(maxdiff, std::fabs(C_openblas[i] - C_alma[i]));
    }
    
    double speedup = t_openblas / t_alma;
    
    std::cout << "OpenBLAS:  " << std::fixed << std::setprecision(2) 
              << t_openblas << " ms (" << gflops(n, t_openblas) << " GFLOPS)\n";
    std::cout << "alma:      " << std::fixed << std::setprecision(2) 
              << t_alma << " ms (" << gflops(n, t_alma) << " GFLOPS)\n";
    std::cout << "Speedup:   " << std::fixed << std::setprecision(2) << speedup << "x\n";
    std::cout << "Max diff:  " << std::scientific << maxdiff << "\n";
    
    return 0;
}
