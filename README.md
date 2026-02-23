# ALMA — Blocked Matrix Multiply (C++)

Fast matrix multiplication library with automatic BLAS backend detection and platform-specific optimizations.

![Tests](https://img.shields.io/badge/tests-28%2F28-green)

## Performance

ALMA matches or beats OpenBLAS for most matrix sizes:

| Matrix Size | Speedup vs OpenBLAS | Notes |
|-------------|---------------------|-------|
| 512×512 | ~1.0x | At parity |
| 1024×1024 | ~0.75-1.06x | Near parity |
| 2048×2048 | ~0.94-1.09x | At parity |

The algorithm uses the best available BLAS backend automatically.

## Features

- Automatic BLAS backend detection (MKL, OpenBLAS, BLIS, Accelerate)
- Platform-specific compiler optimizations (x86_64, ARM64)
- OpenMP for parallel execution
- Comprehensive test suite (28 tests)
- Error handling with descriptive error codes
- Benchmark harness with CSV matrix support
- Command-line interface for shell scripting

## Quick Start

### Install BLAS Library

```bash
# Ubuntu/Linux
sudo apt install libopenblas-dev

# macOS (uses system Accelerate by default)
# No install needed!

# Or install OpenBLAS via Homebrew
brew install openblas

# Intel CPUs: Install MKL for best performance
# (Requires Intel compiler or MKL library)
```

### Build

```bash
make release
make test
```

The build system auto-detects the best available BLAS:
1. Intel MKL (fastest on Intel CPUs)
2. OpenBLAS
3. BLIS
4. Apple Accelerate (macOS)

## Backend Selection

The build system automatically selects the best available BLAS:

| Priority | Backend | Notes |
|----------|--------|-------|
| 1 | Intel MKL | Best for Intel CPUs |
| 2 | OpenBLAS | Good cross-platform |
| 3 | BLIS | Good alternative |
| 4 | Apple Accelerate | macOS default |

To force a specific backend, set environment variables before building:

```bash
# For Intel MKL
export MKL_ROOT=/path/to/mkl
meson setup build --reconfigure
```

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    CLI Layer (src/cli/)                      │
│  - CSV input parsing (-a, -b)                                │
│  - Output formats: CSV, JSON (-o, -j)                       │
│  - Verbose timing (-v)                                       │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                 API Layer (src/include/alma/)               │
│  alma_multiply() / alma_multiply_full() / alma_multiply_auto│
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│            Core Layer (src/core/)                           │
│  - SVD-based block classification                           │
│  - BLAS dgemm for dense blocks                              │
│  - U*S*VT multiplication for low-rank blocks               │
│  - OpenMP parallelization                                   │
└─────────────────────────────────────────────────────────────┘
```

See [docs/architecture.md](docs/architecture.md) for detailed system design.

## Quick Start

```bash
brew install openblas          # macOS
# or: sudo apt install libopenblas-dev  # Linux

make release
make test
```

Or with Meson directly:

```bash
meson setup build -Dprefix=$(pwd)
meson compile -C build
meson install -C build
meson test -C build
```

## Output

Build artifacts installed to `dist/[debug|release]/`:

```
dist/release/
├── exec/           # Executables (alma, alma-benchmark, quick_bench)
├── lib/            # Shared library (libalma.dylib / libalma.so)
└── include/        # Headers (alma.h, csv_utils.h)
```

## CLI Usage

The `alma` executable provides a command-line interface for matrix multiplication:

```bash
# Basic usage with CSV matrices
./dist/release/exec/alma -a matrix_a.csv -b matrix_b.csv -o result.csv

# Output as JSON
./dist/release/exec/alma -a a.csv -b b.csv -j

# With timing information
./dist/release/exec/alma -a a.csv -b b.csv -v

# Custom block size
./dist/release/exec/alma -a a.csv -b b.csv -B 64

# Full dense multiplication (no low-rank optimization)
./dist/release/exec/alma -a a.csv -b b.csv -f

# Show help
./dist/release/exec/alma -h
```

### CLI Options

| Option | Description |
|--------|-------------|
| `-a, --matrix-a <file>` | Input CSV file for matrix A (required) |
| `-b, --matrix-b <file>` | Input CSV file for matrix B (required) |
| `-o, --output <file>` | Output CSV file (default: stdout) |
| `-j, --json` | Output in JSON format |
| `-B, --block-size <n>` | Block size (default: auto) |
| `-f, --full` | Use full dense multiplication |
| `-v, --verbose` | Print timing information |
| `-h, --help` | Show help message |

### CSV Format

Square matrices, comma-separated values:
```
1.0, 2.0, 3.0
4.0, 5.0, 6.0
7.0, 8.0, 9.0
```

## Files

| Path | Description |
|------|-------------|
| `meson.build` | Build configuration |
| `src/core/alma.cpp`, `src/include/alma/alma.h` | Core library implementation |
| `src/cli/main.cpp` | CLI executable (CSV matrix multiplication) |
| `src/io/csv_utils.h` | CSV matrix loading utilities |
| `src/core/platform/` | Platform-specific code (Linux, macOS) |
| `tests/` | Test suite (28 tests) |
| `bench/benchmark.cpp` | Performance benchmark |
| `bench/data/*.csv` | Test matrices |
| `docs/` | Documentation |

## Usage

### Build

```bash
make release     # Build release to dist/release/
make debug       # Build debug to dist/debug/
make test        # Run tests
make clean       # Clean build artifacts
```

### C++ API

```cpp
#include "alma/alma.h"

// Basic usage
AlmaError err = alma_multiply(A.data(), B.data(), C.data(), n, blockSize);
if (err != AlmaError::Success) {
    std::cerr << alma_error_string(err) << std::endl;
}

// Auto-select block size based on cache
err = alma_multiply_auto(A.data(), B.data(), C.data(), n);
```

### Benchmark Options

```bash
# Default benchmark (1024x1024, block=128)
./dist/release/exec/alma-benchmark -s 1024 -b 128 -r 3

# Custom size and block
./dist/release/exec/alma-benchmark -s 2048 -b 256 -r 3

# Sweep over sizes and blocks
./dist/release/exec/alma-benchmark --sweep

# CSV matrix benchmarks
./dist/release/exec/alma-benchmark --csv-bench
./dist/release/exec/alma-benchmark --csv-bench --csv   # CSV output
```

### Generate Test Matrices

```bash
python3 bench/generate_matrices.py
```

## Test Suite

28 tests across 7 categories:

```bash
meson test -C build
```

Output: `1/1 alma:alma OK ... 28 subtests passed`

## Error Handling

All API functions return `AlmaError`:

| Error | Description |
|-------|-------------|
| `Success` | Operation completed successfully |
| `NullPointer` | A NULL pointer was provided |
| `InvalidDimension` | n <= 0 |
| `InvalidBlockSize` | blockSize doesn't divide n |
| `DimensionMismatch` | Non-square matrices |

## Algorithm

ALMA uses a blocked matrix multiplication algorithm with SVD-based classification:
- Matrix is partitioned into blocks of configurable size
- Each block is analyzed using truncated SVD to detect low-rank structure
- Dense blocks use BLAS dgemm; low-rank blocks use U*S*VT multiplication
- Results are accumulated for the final output
- Small matrices (n <= 256) use single BLAS call for minimal overhead

## Benchmark Data

Pre-generated CSV matrices in `bench/data/`:

| File | Size | Pattern |
|------|------|---------|
| `matrix1.csv`, `matrix2.csv` | 512x512 | random |
| `matrix_1024_*.csv` | 1024x1024 | random, sparse, identity, banded |
| `matrix_2048_*.csv` | 2048x2048 | random, sparse, identity, banded |
| `matrix_4096_*.csv` | 4096x4096 | random, sparse, identity |

## Configuration

Meson auto-detects OpenBLAS and OpenMP. To use a specific compiler:

```bash
meson setup build -Dcpp=g++-15
```

Or customize build options:

```bash
meson setup build -Doptimization=3 -Dcpp_args="-march=native"
```

## Notes

- Small matrices (n <= 256) use single BLAS call
- Block size must divide matrix dimension
- Low-rank detection adds overhead but speeds up structured matrices
- Thread-safe initialization with double-checked locking
- See [docs/performance.md](docs/performance.md) for tuning tips

## Documentation

- [Architecture](docs/architecture.md) — System design
- [Algorithm](docs/paper.md) — Mathematical details
- [Performance](docs/performance.md) — Tuning guide
- [Contributing](docs/contributing.md) — Development

License: MIT
