#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <cblas.h>
#include <random>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <functional>
#include <fstream>
#include <sstream>
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

void print_usage(const char* name) {
    std::cout << "Usage: " << name << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -s N          Matrix size (default: 1024)\n";
    std::cout << "  -b N          Block size (default: 128)\n";
    std::cout << "  -r N          Repeats (default: 3)\n";
    std::cout << "  -a FILE       Load matrix A from CSV (default: random)\n";
    std::cout << "  -B FILE       Load matrix B from CSV (default: random)\n";
    std::cout << "  --sweep       Run sweep over sizes and blocks\n";
    std::cout << "  --csv         CSV output mode\n";
    std::cout << "  -v            Verbose output\n";
}

int main(int argc, char** argv) {
    int n = 1024;
    int block = 128;
    int repeats = 3;
    bool sweep = false;
    bool csv = false;
    bool verbose = false;
    std::string matrix_a_path, matrix_b_path;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-s" && i + 1 < argc) {
            n = std::atoi(argv[++i]);
        } else if (arg == "-b" && i + 1 < argc) {
            block = std::atoi(argv[++i]);
        } else if (arg == "-r" && i + 1 < argc) {
            repeats = std::atoi(argv[++i]);
        } else if (arg == "-a" && i + 1 < argc) {
            matrix_a_path = argv[++i];
        } else if (arg == "-B" && i + 1 < argc) {
            matrix_b_path = argv[++i];
        } else if (arg == "--sweep") {
            sweep = true;
        } else if (arg == "--csv") {
            csv = true;
        } else if (arg == "-v") {
            verbose = true;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    std::mt19937 gen(42);
    
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
        if (verbose) {
            std::cout << "Loaded matrices from CSV: " << n << "x" << n << "\n";
        }
    }
    
    if (sweep) {
        // CSV header
        if (csv) {
            std::cout << "n,block,openblas_ms,alma_ms,speedup,gflops_openblas,gflops_alma,maxdiff\n";
        } else {
            std::cout << "========================================\n";
            std::cout << "       BENCHMARK SWEEP\n";
            std::cout << "========================================\n\n";
        }
        
        std::vector<int> sizes = {256, 512, 1024, 2048};
        std::vector<int> blocks = {32, 64, 128, 256};
        
        for (int size : sizes) {
            for (int blk : blocks) {
                if (blk > size) continue;
                
                std::vector<double> A(size*size), B(size*size), C_alma(size*size), C_openblas(size*size);
                random_fill(A, gen);
                random_fill(B, gen);
                
                // Time OpenBLAS
                auto openblas_func = [&]() {
                    std::fill(C_openblas.begin(), C_openblas.end(), 0.0);
                    openblas_mul(A.data(), B.data(), C_openblas.data(), size);
                };
                double t_openblas = time_kernel(openblas_func, repeats);
                
                // Time alma
                auto alma_func = [&]() {
                    std::fill(C_alma.begin(), C_alma.end(), 0.0);
                    alma_multiply(A.data(), B.data(), C_alma.data(), size, blk);
                };
                double t_alma = time_kernel(alma_func, repeats);
                
                // Verify
                double maxdiff = 0.0;
                for (int i = 0; i < size*size; ++i) {
                    maxdiff = std::max(maxdiff, std::fabs(C_openblas[i] - C_alma[i]));
                }
                
                double speedup = t_openblas / t_alma;
                
                if (csv) {
                    std::cout << size << ","
                              << blk << ","
                              << std::fixed << std::setprecision(4) << t_openblas << ","
                              << t_alma << ","
                              << speedup << ","
                              << gflops(size, t_openblas) << ","
                              << gflops(size, t_alma) << ","
                              << std::scientific << maxdiff << "\n";
                } else {
                    std::cout << "Size: " << size << "x" << size << ", Block: " << blk << "\n";
                    std::cout << "  OpenBLAS: " << std::fixed << std::setprecision(2) 
                              << t_openblas << " ms (" << gflops(size, t_openblas) << " GFLOPS)\n";
                    std::cout << "  alma:     " << t_alma << " ms (" << gflops(size, t_alma) << " GFLOPS)\n";
                    std::cout << "  Speedup:  " << speedup << "x\n";
                    std::cout << "  Max diff: " << std::scientific << maxdiff << "\n\n";
                }
            }
        }
    } else {
        // Single benchmark
        std::vector<double> C_alma(n*n), C_openblas(n*n);
        
        if (A.empty()) {
            A.resize(n * n);
            random_fill(A, gen);
        }
        if (B.empty()) {
            B.resize(n * n);
            random_fill(B, gen);
        }
        
        if (verbose) {
            std::cout << "Matrix: " << n << "x" << n << ", Block: " << block << "\n";
            std::cout << "========================================\n";
        }
        
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
        
        if (csv) {
            std::cout << "n,block,openblas_ms,alma_ms,speedup,gflops_openblas,gflops_alma,maxdiff\n";
            std::cout << n << ","
                      << block << ","
                      << std::fixed << std::setprecision(4) << t_openblas << ","
                      << t_alma << ","
                      << speedup << ","
                      << gflops(n, t_openblas) << ","
                      << gflops(n, t_alma) << ","
                      << std::scientific << maxdiff << "\n";
        } else if (!verbose) {
            std::cout << "Matrix: " << n << "x" << n << ", Block: " << block << "\n";
            std::cout << "========================================\n";
        }
        
        if (!csv || verbose) {
            std::cout << "OpenBLAS:  " << std::fixed << std::setprecision(2) 
                      << t_openblas << " ms (" << gflops(n, t_openblas) << " GFLOPS)\n";
            std::cout << "alma:      " << t_alma << " ms (" << gflops(n, t_alma) << " GFLOPS)\n";
            std::cout << "Speedup:   " << speedup << "x\n";
            std::cout << "Max diff:  " << std::scientific << maxdiff << "\n";
        }
    }
    
    return 0;
}
