# Architecture

## Overview

ALMA (Adaptive Linear Matrix Algebra) provides BLAS-accelerated linear algebra operations through both a command-line interface and a C++ API. The library automatically detects and uses the best available BLAS backend (Intel MKL, OpenBLAS, Apple Accelerate, or BLIS).

## System Components

```
┌─────────────────────────────────────────────────────────────┐
│                    CLI Layer (src/cli/)                      │
│  - Subcommands: mul, inv, svd, qr, lu, norm, solve, etc.  │
│  - CSV input/output                                         │
│  - JSON output, verbose timing                              │
└─────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                 API Layer (src/include/alma/)               │
│  - alma_multiply(), alma_inverse(), alma_svd(), etc.       │
│  - Matrix operations: add, scale, transpose                │
│  - Decompositions: LU, QR, SVD                             │
└─────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│              BLAS/LAPACK Layer                              │
│  - cblas_dgemm for matrix multiplication                  │
│  - LAPACKE for factorizations (LU, QR, SVD)               │
│  - OpenMP parallelization                                  │
└─────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│              BLAS Library (OpenBLAS/MKL/Accelerate/BLIS)    │
└─────────────────────────────────────────────────────────────┘
```

## Project Structure

```
src/
├── include/alma/          # Public headers
│   ├── alma.h            # Core API
│   └── alma_cache.h      # Cache/NUMA utilities
├── core/                  # Core implementation
│   ├── alma.cpp         # All linear algebra functions
│   ├── lapacke_android.cpp  # Android LAPACKE implementation
│   └── platform/        # Platform-specific code
│       ├── alma_linux.cpp
│       ├── alma_macos.cpp
│       ├── alma_android.cpp
│       ├── alma_windows.cpp
│       └── alma_ios.cpp
├── io/                   # I/O utilities
│   └── csv_utils.h      # CSV parsing
└── cli/                  # Command-line interface
    └── main.cpp         # CLI executable
```

## CLI Subcommands

| Command | Description | Example |
|---------|-------------|---------|
| `mul`, `matmul` | Matrix multiplication | `alma mul -a A.csv -b B.csv` |
| `add` | Matrix addition | `alma add -a A.csv -b B.csv --alpha 1.0 --beta -1.0` |
| `scale` | Scale matrix | `alma scale -a A.csv --scalar 2.0` |
| `transpose` | Transpose | `alma transpose -a A.csv` |
| `inv`, `inverse` | Matrix inverse | `alma inv -a A.csv -j` |
| `det`, `determinant` | Determinant | `alma det -a A.csv` |
| `norm` | Matrix norm | `alma norm -a A.csv -n fro` |
| `svd` | SVD | `alma svd -a A.csv -o result.json` |
| `lu` | LU decomposition | `alma lu -a A.csv` |
| `qr` | QR decomposition | `alma qr -a A.csv` |
| `solve` | Solve Ax = B | `alma solve -a A.csv -b b.csv` |

## Error Codes

```cpp
enum class AlmaError {
    Success = 0,
    NullPointer,
    InvalidDimension,
    InvalidBlockSize,
    DimensionMismatch,
    SingularMatrix,
    NotImplemented
};
```

## API Reference

### Matrix Multiplication

```cpp
AlmaError alma_multiply(double* A, double* B, double* C, int n, int blockSize);
AlmaError alma_multiply_full(double* A, double* B, double* C, int n, int blockSize);
AlmaError alma_multiply_auto(double* A, double* B, double* C, int n);
```

### Basic Operations

```cpp
AlmaError alma_transpose(double* A, double* AT, int m, int n);
AlmaError alma_add(double* A, double* B, double* C, int m, int n, double alpha, double beta);
AlmaError alma_scale(double* A, int m, int n, double scalar);
```

### Norms

```cpp
enum class NormType { One, Two, Inf, Frobenius };
double alma_norm(double* A, int m, int n, NormType type);
```

### Factorizations

```cpp
AlmaError alma_inverse(double* A, double* invA, int n);
double alma_determinant(double* A, int n);

struct SVDResult {
    double* U;
    double* S;
    double* VT;
    int m, n, k;
};
AlmaError alma_svd(double* A, SVDResult& result, int m, int n);
void alma_svd_free(SVDResult& result);

struct LUResult {
    double* LU;
    int* pivots;
    int n;
};
AlmaError alma_lu(double* A, LUResult& result, int n);
void alma_lu_free(LUResult& result);

struct QRResult {
    double* Q;
    double* R;
    int m, n;
};
AlmaError alma_qr(double* A, QRResult& result, int m, int n);
void alma_qr_free(QRResult& result);
```

### Linear Systems

```cpp
AlmaError alma_solve(double* A, double* B, double* X, int n, int nrhs);
AlmaError alma_solve_lu(const LUResult& lu, double* B, double* X, int n, int nrhs);
```

### Utility

```cpp
int alma_get_optimal_block_size();
const char* alma_error_string(AlmaError err);
```

## Backend Detection

The build system automatically detects and configures BLAS libraries in order of preference:

1. **Intel MKL** - Highest performance on x86_64
2. **OpenBLAS** - Default on Linux
3. **Apple Accelerate** - macOS native

## Build Options

| Option | Description |
|--------|-------------|
| `static-link` | Build with static BLAS for single-file executable |

```bash
# Dynamic build (default)
meson setup build
ninja -C build

# Static build
meson setup build -Dstatic-link=true
ninja -C build
```
