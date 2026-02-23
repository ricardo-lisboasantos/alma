#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <sstream>
#include <memory>
#include <cblas.h>
#include "alma/alma.h"
#include "io/csv_utils.h"

void print_help(const char* prog) {
    std::cout << "ALMA - Adaptive Linear Matrix Algebra\n";
    std::cout << "Bringing BLAS acceleration to the shell\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << prog << " <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  matmul, mul    Matrix multiplication A * B\n";
    std::cout << "  add            Matrix addition alpha*A + beta*B\n";
    std::cout << "  scale          Scale matrix by scalar\n";
    std::cout << "  transpose      Transpose matrix\n";
    std::cout << "  inverse, inv   Matrix inverse\n";
    std::cout << "  determinant    Matrix determinant\n";
    std::cout << "  norm           Matrix norm (1, 2, inf, fro)\n";
    std::cout << "  svd            Singular Value Decomposition\n";
    std::cout << "  qr             QR Decomposition\n";
    std::cout << "  lu             LU Decomposition\n";
    std::cout << "  solve          Solve Ax = B\n";
    std::cout << "  help           Show this help\n\n";
    std::cout << "Options:\n";
    std::cout << "  -a, --a <file>     Input CSV for matrix A (required unless noted)\n";
    std::cout << "  -b, --b <file>     Input CSV for matrix B\n";
    std::cout << "  -o, --output <f>  Output file (default: stdout)\n";
    std::cout << "  -j, --json         JSON output\n";
    std::cout << "  -v, --verbose     Verbose/timing output\n";
    std::cout << "  -n, --norm <t>     Norm type: 1, 2, inf, fro (default: 2)\n";
    std::cout << "  --alpha <val>      Scalar alpha (default: 1.0)\n";
    std::cout << "  --beta <val>       Scalar beta (default: 1.0)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << prog << " mul -a a.csv -b b.csv -o c.csv\n";
    std::cout << "  " << prog << " inv -a matrix.csv -j\n";
    std::cout << "  " << prog << " norm -a matrix.csv -n fro\n";
    std::cout << "  " << prog << " svd -a matrix.csv -o svd.json\n";
}

bool save_csv(const std::string& path, const std::vector<double>& M, int rows, int cols) {
    std::ostream* out;
    std::ofstream file;
    if (path == "-" || path.empty()) {
        out = &std::cout;
    } else {
        file.open(path);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot write to " << path << "\n";
            return false;
        }
        out = &file;
    }
    
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (j > 0) *out << ",";
            *out << std::fixed << std::setprecision(6) << M[i * cols + j];
        }
        *out << "\n";
    }
    return true;
}

void print_json_matrix(const std::string& name, const std::vector<double>& M, int rows, int cols) {
    std::cout << "\"" << name << "\": {\n";
    std::cout << "  \"dims\": [" << rows << ", " << cols << "],\n";
    std::cout << "  \"data\": [\n";
    for (int i = 0; i < rows; ++i) {
        std::cout << "    [";
        for (int j = 0; j < cols; ++j) {
            std::cout << std::fixed << std::setprecision(6) << M[i * cols + j];
            if (j < cols - 1) std::cout << ", ";
        }
        std::cout << "]";
        if (i < rows - 1) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "  ]\n}";
}

void print_json_vector(const std::string& name, const std::vector<double>& v, int n) {
    std::cout << "\"" << name << "\": [";
    for (int i = 0; i < n; ++i) {
        std::cout << std::fixed << std::setprecision(6) << v[i];
        if (i < n - 1) std::cout << ", ";
    }
    std::cout << "]";
}

bool load_matrix(const std::string& path, std::vector<double>& M, int& rows, int& cols) {
    int n;
    bool success = load_csv(path, M, n);
    if (success) {
        rows = n;
        cols = n;
    }
    return success;
}

int cmd_matmul(int argc, char* argv[], bool json) {
    std::string a_path, b_path, output_path;
    bool verbose = false;
    
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--a") == 0) {
            if (i + 1 < argc) a_path = argv[++i];
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--b") == 0) {
            if (i + 1 < argc) b_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) output_path = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    if (a_path.empty() || b_path.empty()) {
        std::cerr << "Error: -a and -b required\n";
        return 1;
    }
    
    std::vector<double> A, B;
    int m, n, p, q;
    if (!load_matrix(a_path, A, m, n)) {
        std::cerr << "Error: Failed to load " << a_path << "\n";
        return 1;
    }
    if (!load_matrix(b_path, B, p, q)) {
        std::cerr << "Error: Failed to load " << b_path << "\n";
        return 1;
    }
    
    if (n != p) {
        std::cerr << "Error: Dimension mismatch: A is " << m << "x" << n 
                  << ", B is " << p << "x" << q << "\n";
        return 1;
    }
    
    std::vector<double> C(m * q, 0.0);
    
    auto start = std::chrono::high_resolution_clock::now();
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                m, q, n, 1.0, A.data(), n, B.data(), q, 0.0, C.data(), q);
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    if (json) {
        std::cout << "{\n";
        print_json_matrix("result", C, m, q);
        std::cout << ",\n\"time_ms\": " << std::fixed << std::setprecision(2) << ms << "\n";
        std::cout << "}\n";
    } else {
        if (verbose) std::cout << "Time: " << ms << " ms\n";
        save_csv(output_path, C, m, q);
    }
    
    return 0;
}

int cmd_add(int argc, char* argv[], bool json) {
    std::string a_path, b_path, output_path;
    double alpha = 1.0, beta = 1.0;
    bool verbose = false;
    
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--a") == 0) {
            if (i + 1 < argc) a_path = argv[++i];
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--b") == 0) {
            if (i + 1 < argc) b_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) output_path = argv[++i];
        } else if (strcmp(argv[i], "--alpha") == 0) {
            if (i + 1 < argc) alpha = std::stod(argv[++i]);
        } else if (strcmp(argv[i], "--beta") == 0) {
            if (i + 1 < argc) beta = std::stod(argv[++i]);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    if (a_path.empty() || b_path.empty()) {
        std::cerr << "Error: -a and -b required\n";
        return 1;
    }
    
    std::vector<double> A, B;
    int m, n, p, q;
    if (!load_matrix(a_path, A, m, n)) return 1;
    if (!load_matrix(b_path, B, p, q)) return 1;
    
    if (m != p || n != q) {
        std::cerr << "Error: Dimension mismatch\n";
        return 1;
    }
    
    std::vector<double> C(m * n);
    alma_add(A.data(), B.data(), C.data(), m, n, alpha, beta);
    
    if (json) {
        std::cout << "{\n";
        print_json_matrix("result", C, m, n);
        std::cout << "\n}\n";
    } else {
        save_csv(output_path, C, m, n);
    }
    return 0;
}

int cmd_scale(int argc, char* argv[], bool json) {
    std::string a_path, output_path;
    double scalar = 2.0;
    bool verbose = false;
    
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--a") == 0) {
            if (i + 1 < argc) a_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) output_path = argv[++i];
        } else if (strcmp(argv[i], "--scalar") == 0 || strcmp(argv[i], "-s") == 0) {
            if (i + 1 < argc) scalar = std::stod(argv[++i]);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    if (a_path.empty()) {
        std::cerr << "Error: -a required\n";
        return 1;
    }
    
    std::vector<double> A;
    int m, n;
    if (!load_matrix(a_path, A, m, n)) return 1;
    
    alma_scale(A.data(), m, n, scalar);
    
    if (json) {
        std::cout << "{\n";
        print_json_matrix("result", A, m, n);
        std::cout << "\n}\n";
    } else {
        save_csv(output_path, A, m, n);
    }
    return 0;
}

int cmd_transpose(int argc, char* argv[], bool json) {
    std::string a_path, output_path;
    bool verbose = false;
    
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--a") == 0) {
            if (i + 1 < argc) a_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) output_path = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    if (a_path.empty()) {
        std::cerr << "Error: -a required\n";
        return 1;
    }
    
    std::vector<double> A;
    int m, n;
    if (!load_matrix(a_path, A, m, n)) return 1;
    
    std::vector<double> AT(n * m);
    alma_transpose(A.data(), AT.data(), m, n);
    
    if (json) {
        std::cout << "{\n";
        print_json_matrix("result", AT, n, m);
        std::cout << "\n}\n";
    } else {
        save_csv(output_path, AT, n, m);
    }
    return 0;
}

int cmd_inverse(int argc, char* argv[], bool json) {
    std::string a_path, output_path;
    bool verbose = false;
    
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--a") == 0) {
            if (i + 1 < argc) a_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) output_path = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    if (a_path.empty()) {
        std::cerr << "Error: -a required\n";
        return 1;
    }
    
    std::vector<double> A;
    int n;
    if (!load_matrix(a_path, A, n, n)) return 1;
    
    std::vector<double> invA(n * n);
    auto start = std::chrono::high_resolution_clock::now();
    AlmaError err = alma_inverse(A.data(), invA.data(), n);
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    if (err != AlmaError::Success) {
        std::cerr << "Error: " << alma_error_string(err) << "\n";
        return 1;
    }
    
    if (json) {
        std::cout << "{\n";
        print_json_matrix("inverse", invA, n, n);
        std::cout << ",\n\"time_ms\": " << std::fixed << std::setprecision(2) << ms << "\n";
        std::cout << "\n}\n";
    } else {
        if (verbose) std::cout << "Time: " << ms << " ms\n";
        save_csv(output_path, invA, n, n);
    }
    return 0;
}

int cmd_determinant(int argc, char* argv[], bool json) {
    std::string a_path;
    bool verbose = false;
    
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--a") == 0) {
            if (i + 1 < argc) a_path = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    if (a_path.empty()) {
        std::cerr << "Error: -a required\n";
        return 1;
    }
    
    std::vector<double> A;
    int n;
    if (!load_matrix(a_path, A, n, n)) return 1;
    
    auto start = std::chrono::high_resolution_clock::now();
    double det = alma_determinant(A.data(), n);
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    if (json) {
        std::cout << "{\n";
        std::cout << "  \"determinant\": " << std::scientific << det << ",\n";
        std::cout << "  \"time_ms\": " << std::fixed << std::setprecision(2) << ms << "\n";
        std::cout << "}\n";
    } else {
        if (verbose) std::cout << "Time: " << ms << " ms\n";
        std::cout << std::scientific << det << "\n";
    }
    return 0;
}

int cmd_norm(int argc, char* argv[], bool json) {
    std::string a_path;
    std::string norm_str = "2";
    bool verbose = false;
    
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--a") == 0) {
            if (i + 1 < argc) a_path = argv[++i];
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--norm") == 0) {
            if (i + 1 < argc) norm_str = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    if (a_path.empty()) {
        std::cerr << "Error: -a required\n";
        return 1;
    }
    
    std::vector<double> A;
    int m, n;
    if (!load_matrix(a_path, A, m, n)) return 1;
    
    NormType type = NormType::Two;
    if (norm_str == "1") type = NormType::One;
    else if (norm_str == "inf" || norm_str == "i") type = NormType::Inf;
    else if (norm_str == "fro" || norm_str == "f") type = NormType::Frobenius;
    
    double norm = alma_norm(A.data(), m, n, type);
    
    if (json) {
        std::cout << "{\n";
        std::cout << "  \"norm\": " << std::scientific << norm << ",\n";
        std::cout << "  \"type\": \"" << norm_str << "\"\n";
        std::cout << "}\n";
    } else {
        std::cout << std::scientific << norm << "\n";
    }
    return 0;
}

int cmd_svd(int argc, char* argv[], bool json) {
    std::string a_path, output_path;
    bool verbose = false;
    
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--a") == 0) {
            if (i + 1 < argc) a_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) output_path = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    if (a_path.empty()) {
        std::cerr << "Error: -a required\n";
        return 1;
    }
    
    std::vector<double> A;
    int m, n;
    if (!load_matrix(a_path, A, m, n)) return 1;
    
    SVDResult result = {nullptr, nullptr, nullptr, 0, 0, 0};
    auto start = std::chrono::high_resolution_clock::now();
    AlmaError err = alma_svd(A.data(), result, m, n);
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    if (err != AlmaError::Success) {
        std::cerr << "Error: " << alma_error_string(err) << "\n";
        return 1;
    }
    
    if (json) {
        std::cout << "{\n";
        std::cout << "  \"U\": {\"dims\": [" << result.m << ", " << result.k << "], \"data\": [";
        for (int i = 0; i < result.m * result.k; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << std::fixed << std::setprecision(6) << result.U[i];
        }
        std::cout << "]},\n";
        
        std::cout << "  \"S\": [";
        for (int i = 0; i < result.k; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << std::scientific << result.S[i];
        }
        std::cout << "],\n";
        
        std::cout << "  \"VT\": {\"dims\": [" << result.k << ", " << result.n << "], \"data\": [";
        for (int i = 0; i < result.k * result.n; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << std::fixed << std::setprecision(6) << result.VT[i];
        }
        std::cout << "]},\n";
        
        std::cout << "  \"time_ms\": " << std::fixed << std::setprecision(2) << ms << "\n";
        std::cout << "}\n";
    } else {
        if (verbose) std::cout << "Time: " << ms << " ms\n";
        std::cout << "Singular values (" << result.k << "):\n";
        for (int i = 0; i < result.k; ++i) {
            std::cout << "  " << i << ": " << std::scientific << result.S[i] << "\n";
        }
        if (!output_path.empty()) {
            std::ofstream f(output_path);
            f << "U:\n";
            for (int i = 0; i < result.m; ++i) {
                for (int j = 0; j < result.k; ++j) {
                    if (j > 0) f << ",";
                    f << std::fixed << std::setprecision(6) << result.U[i * result.k + j];
                }
                f << "\n";
            }
            f << "\nS:\n";
            for (int i = 0; i < result.k; ++i) {
                f << result.S[i] << "\n";
            }
            f << "\nVT:\n";
            for (int i = 0; i < result.k; ++i) {
                for (int j = 0; j < result.n; ++j) {
                    if (j > 0) f << ",";
                    f << std::fixed << std::setprecision(6) << result.VT[i * result.n + j];
                }
                f << "\n";
            }
        }
    }
    
    alma_svd_free(result);
    return 0;
}

int cmd_lu(int argc, char* argv[], bool json) {
    std::string a_path, output_path;
    bool verbose = false;
    
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--a") == 0) {
            if (i + 1 < argc) a_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) output_path = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    if (a_path.empty()) {
        std::cerr << "Error: -a required\n";
        return 1;
    }
    
    std::vector<double> A;
    int n;
    if (!load_matrix(a_path, A, n, n)) return 1;
    
    LUResult result = {nullptr, nullptr, 0};
    auto start = std::chrono::high_resolution_clock::now();
    AlmaError err = alma_lu(A.data(), result, n);
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    if (err != AlmaError::Success) {
        std::cerr << "Error: " << alma_error_string(err) << "\n";
        return 1;
    }
    
    if (json) {
        std::cout << "{\n";
        print_json_matrix("LU", std::vector<double>(result.LU, result.LU + n * n), n, n);
        std::cout << ",\n\"pivots\": [";
        for (int i = 0; i < n; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << result.pivots[i];
        }
        std::cout << "],\n\"time_ms\": " << std::fixed << std::setprecision(2) << ms << "\n";
        std::cout << "}\n";
    } else {
        if (verbose) std::cout << "Time: " << ms << " ms\n";
        if (!output_path.empty()) {
            save_csv(output_path, std::vector<double>(result.LU, result.LU + n * n), n, n);
        } else {
            std::cout << "LU decomposition:\n";
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    if (j > 0) std::cout << ", ";
                    std::cout << std::fixed << std::setprecision(4) << result.LU[i * n + j];
                }
                std::cout << "\n";
            }
            std::cout << "Pivots: ";
            for (int i = 0; i < n; ++i) std::cout << result.pivots[i] << " ";
            std::cout << "\n";
        }
    }
    
    alma_lu_free(result);
    return 0;
}

int cmd_qr(int argc, char* argv[], bool json) {
    std::string a_path, output_path;
    bool verbose = false;
    
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--a") == 0) {
            if (i + 1 < argc) a_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) output_path = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    if (a_path.empty()) {
        std::cerr << "Error: -a required\n";
        return 1;
    }
    
    std::vector<double> A;
    int m, n;
    if (!load_matrix(a_path, A, m, n)) return 1;
    
    QRResult result = {nullptr, nullptr, 0, 0};
    auto start = std::chrono::high_resolution_clock::now();
    AlmaError err = alma_qr(A.data(), result, m, n);
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    if (err != AlmaError::Success) {
        std::cerr << "Error: " << alma_error_string(err) << "\n";
        return 1;
    }
    
    if (json) {
        std::cout << "{\n";
        print_json_matrix("Q", std::vector<double>(result.Q, result.Q + m * m), m, m);
        std::cout << ",\n";
        print_json_matrix("R", std::vector<double>(result.R, result.R + m * n), m, n);
        std::cout << ",\n\"time_ms\": " << std::fixed << std::setprecision(2) << ms << "\n";
        std::cout << "}\n";
    } else {
        if (verbose) std::cout << "Time: " << ms << " ms\n";
        std::cout << "Q (" << m << "x" << m << "):\n";
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < m; ++j) {
                if (j > 0) std::cout << ", ";
                std::cout << std::fixed << std::setprecision(4) << result.Q[i * m + j];
            }
            std::cout << "\n";
        }
        std::cout << "R (" << m << "x" << n << "):\n";
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < n; ++j) {
                if (j > 0) std::cout << ", ";
                std::cout << std::fixed << std::setprecision(4) << result.R[i * n + j];
            }
            std::cout << "\n";
        }
    }
    
    alma_qr_free(result);
    return 0;
}

int cmd_solve(int argc, char* argv[], bool json) {
    std::string a_path, b_path, output_path;
    bool verbose = false;
    
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--a") == 0) {
            if (i + 1 < argc) a_path = argv[++i];
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--b") == 0) {
            if (i + 1 < argc) b_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) output_path = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    if (a_path.empty() || b_path.empty()) {
        std::cerr << "Error: -a and -b required\n";
        return 1;
    }
    
    std::vector<double> A, B;
    int n, nrhs;
    if (!load_matrix(a_path, A, n, n)) return 1;
    if (!load_matrix(b_path, B, nrhs, nrhs)) {
        std::vector<double> b_vec;
        if (load_csv(b_path, b_vec, nrhs)) {
            B = b_vec;
            nrhs = 1;
        } else {
            return 1;
        }
    }
    
    std::vector<double> X(n * nrhs);
    auto start = std::chrono::high_resolution_clock::now();
    AlmaError err = alma_solve(A.data(), B.data(), X.data(), n, nrhs);
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    if (err != AlmaError::Success) {
        std::cerr << "Error: " << alma_error_string(err) << "\n";
        return 1;
    }
    
    if (json) {
        std::cout << "{\n";
        print_json_matrix("solution", X, n, nrhs);
        std::cout << ",\n\"time_ms\": " << std::fixed << std::setprecision(2) << ms << "\n";
        std::cout << "}\n";
    } else {
        if (verbose) std::cout << "Time: " << ms << " ms\n";
        save_csv(output_path, X, n, nrhs);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 0;
    }
    
    std::string cmd = argv[1];
    bool json = false;
    
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "-j") == 0 || strcmp(argv[i], "--json") == 0) {
            json = true;
        }
    }
    
    int cmd_argc = argc - 2;
    char** cmd_argv = &argv[2];
    
    if (cmd == "help" || cmd == "-h" || cmd == "--help") {
        print_help(argv[0]);
        return 0;
    } else if (cmd == "matmul" || cmd == "mul" || cmd == "multiply") {
        return cmd_matmul(cmd_argc, cmd_argv, json);
    } else if (cmd == "add") {
        return cmd_add(cmd_argc, cmd_argv, json);
    } else if (cmd == "scale") {
        return cmd_scale(cmd_argc, cmd_argv, json);
    } else if (cmd == "transpose" || cmd == "trans") {
        return cmd_transpose(cmd_argc, cmd_argv, json);
    } else if (cmd == "inverse" || cmd == "inv") {
        return cmd_inverse(cmd_argc, cmd_argv, json);
    } else if (cmd == "determinant" || cmd == "det") {
        return cmd_determinant(cmd_argc, cmd_argv, json);
    } else if (cmd == "norm") {
        return cmd_norm(cmd_argc, cmd_argv, json);
    } else if (cmd == "svd") {
        return cmd_svd(cmd_argc, cmd_argv, json);
    } else if (cmd == "lu") {
        return cmd_lu(cmd_argc, cmd_argv, json);
    } else if (cmd == "qr") {
        return cmd_qr(cmd_argc, cmd_argv, json);
    } else if (cmd == "solve") {
        return cmd_solve(cmd_argc, cmd_argv, json);
    } else {
        std::cerr << "Unknown command: " << cmd << "\n";
        print_help(argv[0]);
        return 1;
    }
    
    return 0;
}
