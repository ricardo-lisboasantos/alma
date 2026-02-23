# ALMA Documentation

Bringing BLAS-accelerated linear algebra to the shell.

## Quick Start

```bash
# Build
meson setup build
ninja -C build

# Run
./build/alma help
```

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

# Norm calculation (1, 2, inf, fro)
./build/alma norm -a matrix.csv -n fro

# Determinant
./build/alma det -a matrix.csv

# LU/QR decomposition
./build/alma lu -a matrix.csv
./build/alma qr -a matrix.csv
```

## All Commands

| Command | Description |
|---------|-------------|
| `mul`, `matmul` | Matrix multiplication A * B |
| `add` | Matrix addition alpha*A + beta*B |
| `scale` | Scale matrix by scalar |
| `transpose` | Transpose matrix |
| `inv`, `inverse` | Matrix inverse |
| `det`, `determinant` | Matrix determinant |
| `norm` | Matrix norm (1, 2, inf, fro) |
| `svd` | Singular Value Decomposition |
| `qr` | QR Decomposition |
| `lu` | LU Decomposition |
| `solve` | Solve Ax = B |

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

## Documentation

| Guide | Description |
|-------|-------------|
| [High-Level Design](high_level.md) | System architecture, components, data flow |
| [Low-Level Design](low_level.md) | Data structures, algorithms, implementation details |
| [Architecture](architecture.md) | System design, components, API reference |
| [Algorithm](paper.md) | Mathematical background and complexity |
| [Performance](performance.md) | Benchmarking, tuning, optimization |
| [Contributing](contributing.md) | Development setup, code style |

## Building

### Standard Build

```bash
meson setup build
ninja -C build
```

### Static Build (Single-File Executable)

```bash
meson setup build -Dstatic-link=true
ninja -C build
```

Note: Requires static OpenBLAS libraries installed on the system.

### Cross-Platform Builds

#### Android (arm64-v8a)

Requires Android NDK. Set `ANDROID_NDK_ROOT` environment variable:

```bash
export ANDROID_NDK_ROOT=/path/to/android-ndk
make android
```

The Android build uses BLIS as the BLAS backend with a custom LAPACKE implementation for LAPACK operations. All dependencies are statically linked.

#### iOS

Requires Xcode. Build with:

```bash
make ios
```

#### Windows (MinGW)

Requires MinGW-w64. Build with:

```bash
make windows
```

### Test Suite

```bash
meson test -C build
```

## Benchmarks

```bash
# Default: matrix multiplication
./build/alma-benchmark -s 1024

# Different operations
./build/alma-benchmark -o lu -s 1024   # LU decomposition
./build/alma-benchmark -o qr -s 1024    # QR decomposition
./build/alma-benchmark -o svd -s 512    # SVD
./build/alma-benchmark -o inv -s 1024   # Matrix inverse

# Size sweep
./build/alma-benchmark --sweep -o mul   # Sweep multiplication
./build/alma-benchmark --sweep -o all    # Sweep all operations

# CSV output
./build/alma-benchmark -o mul --csv

# System info
./build/alma-benchmark --sysinfo
```

## Backend Detection

ALMA automatically detects and uses the best available BLAS backend:

- **Intel MKL** (highest performance on x86)
- **OpenBLAS** (default on Linux)
- **Apple Accelerate** (macOS)
- **BLIS** (Android arm64, optimized for ARM)
