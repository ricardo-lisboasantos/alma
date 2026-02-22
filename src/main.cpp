#include <iostream>
#include <vector>
#include <iomanip>
#include "alma.h"

int main() {
    int n = 512;
    int block = 128;

    std::vector<double> A(n*n), B(n*n), C(n*n, 0.0);

    for (int i = 0; i < n*n; ++i) {
        A[i] = (i % 7) * 0.1;
        B[i] = (i % 5) * 0.2;
    }

    alma_multiply(A.data(), B.data(), C.data(), n, block);

    std::cout << "{\n";
    std::cout << "  \"dims\": [" << n << ", " << n << "],\n";
    std::cout << "  \"size\": " << n * n << ",\n";
    std::cout << "  \"data\": [\n";
    for (int i = 0; i < n; ++i) {
        std::cout << "    [";
        for (int j = 0; j < n; ++j) {
            std::cout << std::fixed << std::setprecision(6) << C[i * n + j];
            if (j < n - 1) std::cout << ", ";
        }
        std::cout << "]";
        if (i < n - 1) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "  ]\n";
    std::cout << "}\n";
    return 0;
}
