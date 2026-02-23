#include "test_util.h"
#include <cblas.h>

namespace {

bool test_inverse() {
    std::vector<double> A = {4, 2, 3, 3, 5, 4, 2, 1, 8};
    std::vector<double> invA(9);
    
    AlmaError err = alma_inverse(A.data(), invA.data(), 3);
    if (err != AlmaError::Success) {
        std::cerr << "alma_inverse failed\n";
        return false;
    }
    
    std::vector<double> I(9, 0.0);
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 3, 3, 3, 1.0, A.data(), 3, invA.data(), 3, 0.0, I.data(), 3);
    
    double tolerance = 1e-6;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            double expected = (i == j) ? 1.0 : 0.0;
            if (std::fabs(I[i*3 + j] - expected) > tolerance) {
                std::cerr << "Inverse mismatch at [" << i << "," << j << "]: " << I[i*3 + j] << " vs " << expected << "\n";
                return false;
            }
        }
    }
    return true;
}

bool test_determinant() {
    std::vector<double> A = {4, 2, 3, 3, 5, 4, 2, 1, 8};
    double det = alma_determinant(A.data(), 3);
    double expected = 91.0;
    
    if (std::fabs(det - expected) > 1e-6) {
        std::cerr << "Determinant mismatch: " << det << " vs " << expected << "\n";
        return false;
    }
    return true;
}

bool test_determinant_singular() {
    std::vector<double> A = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    double det = alma_determinant(A.data(), 3);
    
    if (std::fabs(det) > 1e-10) {
        std::cerr << "Singular matrix determinant should be 0: " << det << "\n";
        return false;
    }
    return true;
}

bool test_norm_fro() {
    std::vector<double> A = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    double norm = alma_norm(A.data(), 3, 3, NormType::Frobenius);
    double expected = std::sqrt(285.0);
    
    if (std::fabs(norm - expected) > 1e-6) {
        std::cerr << "Frobenius norm mismatch: " << norm << " vs " << expected << "\n";
        return false;
    }
    return true;
}

bool test_norm_one() {
    std::vector<double> A = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    double norm = alma_norm(A.data(), 3, 3, NormType::One);
    double expected = 18.0;
    
    if (std::fabs(norm - expected) > 1e-6) {
        std::cerr << "One-norm mismatch: " << norm << " vs " << expected << "\n";
        return false;
    }
    return true;
}

bool test_norm_inf() {
    std::vector<double> A = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    double norm = alma_norm(A.data(), 3, 3, NormType::Inf);
    double expected = 24.0;
    
    if (std::fabs(norm - expected) > 1e-6) {
        std::cerr << "Inf-norm mismatch: " << norm << " vs " << expected << "\n";
        return false;
    }
    return true;
}

bool test_transpose() {
    std::vector<double> A = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<double> AT(9);
    
    AlmaError err = alma_transpose(A.data(), AT.data(), 3, 3);
    if (err != AlmaError::Success) {
        std::cerr << "alma_transpose failed\n";
        return false;
    }
    
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (AT[i*3 + j] != A[j*3 + i]) {
                std::cerr << "Transpose mismatch at [" << i << "," << j << "]\n";
                return false;
            }
        }
    }
    return true;
}

bool test_add() {
    std::vector<double> A = {1, 2, 3, 4};
    std::vector<double> B = {5, 6, 7, 8};
    std::vector<double> C(4);
    
    AlmaError err = alma_add(A.data(), B.data(), C.data(), 2, 2, 1.0, 1.0);
    if (err != AlmaError::Success) {
        std::cerr << "alma_add failed\n";
        return false;
    }
    
    double expected[] = {6, 8, 10, 12};
    for (int i = 0; i < 4; ++i) {
        if (std::fabs(C[i] - expected[i]) > 1e-6) {
            std::cerr << "Add mismatch at " << i << ": " << C[i] << " vs " << expected[i] << "\n";
            return false;
        }
    }
    return true;
}

bool test_scale() {
    std::vector<double> A = {1, 2, 3, 4};
    
    AlmaError err = alma_scale(A.data(), 2, 2, 2.0);
    if (err != AlmaError::Success) {
        std::cerr << "alma_scale failed\n";
        return false;
    }
    
    double expected[] = {2, 4, 6, 8};
    for (int i = 0; i < 4; ++i) {
        if (std::fabs(A[i] - expected[i]) > 1e-6) {
            std::cerr << "Scale mismatch at " << i << ": " << A[i] << " vs " << expected[i] << "\n";
            return false;
        }
    }
    return true;
}

bool test_lu() {
    std::vector<double> A = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    LUResult result = {nullptr, nullptr, 0};
    
    AlmaError err = alma_lu(A.data(), result, 3);
    if (err != AlmaError::Success) {
        std::cerr << "alma_lu failed\n";
        return false;
    }
    
    alma_lu_free(result);
    return true;
}

bool test_svd() {
    std::vector<double> A = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    SVDResult result = {nullptr, nullptr, nullptr, 0, 0, 0};
    
    AlmaError err = alma_svd(A.data(), result, 3, 3);
    if (err != AlmaError::Success) {
        std::cerr << "alma_svd failed\n";
        return false;
    }
    
    alma_svd_free(result);
    return true;
}

bool test_qr() {
    std::vector<double> A = {1, 2, 3, 4, 5, 6};
    QRResult result = {nullptr, nullptr, 0, 0};
    
    AlmaError err = alma_qr(A.data(), result, 2, 3);
    if (err != AlmaError::Success) {
        std::cerr << "alma_qr failed\n";
        return false;
    }
    
    alma_qr_free(result);
    return true;
}

bool test_solve() {
    std::vector<double> A = {4, 2, 3, 3, 5, 4, 2, 1, 8};
    std::vector<double> B = {1, 1, 1};
    std::vector<double> X(3);
    
    AlmaError err = alma_solve(A.data(), B.data(), X.data(), 3, 1);
    if (err != AlmaError::Success) {
        std::cerr << "alma_solve failed\n";
        return false;
    }
    
    std::vector<double> AX(3);
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 3, 1, 3, 1.0, A.data(), 3, X.data(), 1, 0.0, AX.data(), 1);
    
    double tolerance = 1e-6;
    for (int i = 0; i < 3; ++i) {
        if (std::fabs(AX[i] - B[i]) > tolerance) {
            std::cerr << "Solve mismatch at " << i << ": " << AX[i] << " vs " << B[i] << "\n";
            return false;
        }
    }
    return true;
}

}

REGISTER_TEST("inverse 3x3", test_inverse);
REGISTER_TEST("determinant", test_determinant);
REGISTER_TEST("determinant singular", test_determinant_singular);
REGISTER_TEST("norm frobenius", test_norm_fro);
REGISTER_TEST("norm one", test_norm_one);
REGISTER_TEST("norm inf", test_norm_inf);
REGISTER_TEST("transpose", test_transpose);
REGISTER_TEST("add", test_add);
REGISTER_TEST("scale", test_scale);
REGISTER_TEST("lu decomposition", test_lu);
REGISTER_TEST("svd", test_svd);
REGISTER_TEST("qr decomposition", test_qr);
REGISTER_TEST("solve linear system", test_solve);
