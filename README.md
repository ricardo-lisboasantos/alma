# ALMA — Adaptive Linear Matrix Algebra

BLAS-accelerated linear algebra for the shell. Bringing matrix operations to the command line.

![Tests](https://img.shields.io/badge/tests-28%2F28-green)
![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS%20%7C%20Windows%20%7C%20Android%20%7C%20iOS-green)

## Quick Start

```bash
# Build
meson setup build
ninja -C build

# Run
./build/alma help

# Test
meson test -C build
```

## Features

- **Full Linear Algebra**: mul, inv, det, norm, svd, qr, lu, solve
- **Multi-Platform**: Linux, macOS, Windows, Android, iOS
- **BLAS Backend**: Auto-detects MKL, OpenBLAS, Accelerate
- **Shell-Ready**: CSV I/O, JSON output, pipeable commands
- **Static Linking**: Single-file executable option

## CLI Usage

```bash
# Matrix multiplication
./build/alma mul -a a.csv -b b.csv -o result.csv

# Matrix inverse
./build/alma inv -a matrix.csv -j

# Solve Ax = B
./build/alma solve -a A.csv -b b.csv -o x.csv

# SVD decomposition
./build/alma svd -a matrix.csv -j

# Norm (1, 2, inf, fro)
./build/alma norm -a matrix.csv -n fro

# Determinant
./build/alma det -a matrix.csv

# LU/QR decomposition
./build/alma lu -a matrix.csv
./build/alma qr -a matrix.csv

# Transpose
./build/alma transpose -a matrix.csv

# Scale
./build/alma scale -a matrix.csv --scalar 2.0

# Add (alpha*A + beta*B)
./build/alma add -a a.csv -b b.csv --alpha 1.0 --beta -1.0
```

## All Commands

| Command | Aliases | Description |
|---------|---------|-------------|
| `mul` | matmult, multiply | Matrix multiplication A * B |
| `add` | add | Matrix addition alpha*A + beta*B |
| `scale` | scale | Scale matrix by scalar |
| `transpose` | trans | Transpose matrix |
| `inv` | inverse | Matrix inverse |
| `det` | determinant | Matrix determinant |
| `norm` | norm | Matrix norm (1, 2, inf, fro) |
| `svd` | svd | Singular Value Decomposition |
| `qr` | qr | QR Decomposition |
| `lu` | lu | LU Decomposition |
| `solve` | solve | Solve Ax = B |

## Options

| Flag | Description |
|------|-------------|
| `-a`, `--a` | Input CSV file for matrix A |
| `-b`, `--b` | Input CSV file for matrix B |
| `-o`, `--output` | Output file (default: stdout) |
| `-j`, `--json` | JSON output format |
| `-v`, `--verbose` | Print timing information |
| `-n`, `--norm` | Norm type: 1, 2, inf, fro |
| `--alpha` | Scalar alpha (default: 1.0) |
| `--beta` | Scalar beta (default: 1.0) |
| `--scalar` | Scalar for scale command |

## Installation

### Linux

```bash
sudo apt install libopenblas-dev liblapacke-dev
meson setup build
ninja -C build
sudo ninja -C build install
```

### macOS

```bash
# Uses system Accelerate framework automatically
meson setup build
ninja -C build
sudo ninja -C build install
```

### Windows

```bash
# Install OpenBLAS or MKL
meson setup build
ninja -C build
```

### Static Build (Single File)

```bash
meson setup build -Dstatic-link=true
ninja -C build
```

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    CLI Layer                                  │
│  Subcommands: mul, inv, svd, qr, lu, norm, solve, etc.    │
│  CSV/JSON I/O, verbose timing                              │
└─────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                 API Layer (C++)                              │
│  alma_multiply(), alma_inverse(), alma_svd(), etc.        │
│  Matrix operations, decompositions, linear systems        │
└─────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│              BLAS/LAPACK Backend                             │
│  cblas_dgemm, LAPACKE factorizations                       │
│  OpenMP parallelization                                    │
└─────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│              BLAS Library                                    │
│  OpenBLAS / Intel MKL / Apple Accelerate                   │
└─────────────────────────────────────────────────────────────┘
```

## Platform Support

| Platform | Backend | Status |
|----------|---------|--------|
| Linux x86_64 | OpenBLAS/MKL | Supported |
| macOS | Accelerate | Supported |
| Windows | OpenBLAS/MKL | Supported |
| Android | OpenBLAS | Supported |
| iOS | Accelerate | Supported |

## Benchmarks

```bash
# Matrix multiplication
./build/alma-benchmark -o mul -s 1024

# LU decomposition
./build/alma-benchmark -o lu -s 1024

# SVD
./build/alma-benchmark -o svd -s 512

# Size sweep
./build/alma-benchmark --sweep -o mul

# All operations
./build/alma-benchmark --sweep -o all

# CSV output
./build/alma-benchmark -o mul --csv

# System info
./build/alma-benchmark --sysinfo
```

## C++ API

```cpp
#include "alma/alma.h"

// Matrix multiplication
alma_multiply(A.data(), B.data(), C.data(), n, blockSize);

// Matrix inverse
alma_inverse(A.data(), invA.data(), n);

// Determinant
double det = alma_determinant(A.data(), n);

// Norm
double norm = alma_norm(A.data(), m, n, NormType::Frobenius);

// SVD
SVDResult result;
alma_svd(A.data(), result, m, n);
// Use result.U, result.S, result.VT
alma_svd_free(result);

// LU decomposition
LUResult lu;
alma_lu(A.data(), lu, n);
// Use lu.LU, lu.pivots
alma_lu_free(lu);

// Solve Ax = B
alma_solve(A.data(), B.data(), X.data(), n, nrhs);
```

## CSV Format

Square matrices, comma-separated values:
```
1.0, 2.0, 3.0
4.0, 5.0, 6.0
7.0, 8.0, 9.0
```

## Test Suite

```bash
meson test -C build
```

Output: `1/1 alma OK ... 28 subtests passed`

## Error Handling

All API functions return `AlmaError`:

| Error | Description |
|-------|-------------|
| `Success` | Operation completed successfully |
| `NullPointer` | A NULL pointer was provided |
| `InvalidDimension` | Invalid matrix dimension |
| `InvalidBlockSize` | Invalid block size |
| `DimensionMismatch` | Matrix dimension mismatch |
| `SingularMatrix` | Matrix is singular (non-invertible) |
| `NotImplemented` | Operation not implemented |

## Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `static-link` | Static BLAS linking | false |
| `buildtype` | Debug or release | release |
| `cpp_std` | C++ standard | c++17 |

```bash
# Debug build
meson setup build -Dbuildtype=debug

# Static build for single-file executable
meson setup build -Dstatic-link=true

# Custom compiler
meson setup build -Dcpp=clang++
```

## Files

| Path | Description |
|------|-------------|
| `meson.build` | Build configuration |
| `meson_options.txt` | Build options |
| `src/core/alma.cpp` | Core implementation |
| `src/include/alma/alma.h` | Public API |
| `src/cli/main.cpp` | CLI executable |
| `src/io/csv_utils.h` | CSV utilities |
| `src/core/platform/` | Platform-specific code |
| `tests/` | Test suite (28 tests) |
| `bench/benchmark.cpp` | Benchmark tool |
| `docs/` | Documentation |

## Documentation

- [Architecture](docs/architecture.md) — System design
- [Algorithm](docs/paper.md) — Mathematical details
- [Performance](docs/performance.md) — Tuning guide
- [Contributing](docs/contributing.md) — Development

License: MIT
