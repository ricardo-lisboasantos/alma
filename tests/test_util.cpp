#include "test_util.h"
#include <fstream>
#include <sstream>

void ref_mul(const double* A, const double* B, double* C, int n) {
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

bool test_matrix(int n, int block, const std::vector<double>& A_in, const std::vector<double>& B_in) {
    std::vector<double> A = A_in;
    std::vector<double> B = B_in;
    std::vector<double> C(n*n, 0.0), R(n*n, 0.0);
    
    ref_mul(A.data(), B.data(), R.data(), n);
    alma_multiply(A.data(), B.data(), C.data(), n, block);
    
    double tolerance = 1e-9;
    for (int i = 0; i < n*n; ++i) {
        if (std::fabs(R[i] - C[i]) > tolerance) {
            std::cerr << "Mismatch at " << i << ": ref=" << R[i] << " got=" << C[i] << "\n";
            return false;
        }
    }
    return true;
}
