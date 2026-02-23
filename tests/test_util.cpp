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

bool test_matrix(int n, int block, const std::vector<double>& A_in, const std::vector<double>& B_in) {
    std::vector<double> A = A_in;
    std::vector<double> B = B_in;
    std::vector<double> C(n*n, 0.0), R(n*n, 0.0);
    
    ref_mul(A.data(), B.data(), R.data(), n);
    AlmaError err = alma_multiply(A.data(), B.data(), C.data(), n, block);
    if (err != AlmaError::Success) {
        std::cerr << "alma_multiply failed: " << alma_error_string(err) << "\n";
        return false;
    }
    
    double tolerance = 1e-9;
    for (int i = 0; i < n*n; ++i) {
        if (std::fabs(R[i] - C[i]) > tolerance) {
            std::cerr << "Mismatch at " << i << ": ref=" << R[i] << " got=" << C[i] << "\n";
            return false;
        }
    }
    return true;
}
