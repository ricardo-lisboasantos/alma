# ALMA — Blocked Matrix Multiply (C++)

Fast blocked matrix multiplication with SVD-based low-rank block detection and OpenBLAS/BLAS acceleration.

![Tests](https://img.shields.io/badge/tests-28%2F28-green)

## Features

- SVD-based low-rank block detection for structured matrices
- BLAS (OpenBLAS) for high-performance dense kernels
- LAPACK integration for singular value decomposition
- OpenMP for parallel execution
- Comprehensive test suite (28 tests)
- Error handling with descriptive error codes
- Benchmark harness with CSV matrix support

## Architecture

![Architecture Flowchart](docs/diagrams/architecture.mmd#flowchart)
![Class Diagram](docs/diagrams/architecture.mmd#class)
![Sequence Diagram](docs/diagrams/architecture.mmd#sequence)

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

## Files

| Path | Description |
|------|-------------|
| `meson.build` | Build configuration |
| `src/alma.cpp`, `src/alma.h` | Core implementation |
| `src/main.cpp` | Example program (JSON output) |
| `src/csv_utils.h` | CSV matrix loading utilities |
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
#include "alma.h"

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
./build/alma-benchmark -s 1024 -b 128 -r 3

# Custom size and block
./build/alma-benchmark -s 2048 -b 256 -r 3

# Sweep over sizes and blocks
./build/alma-benchmark --sweep

# CSV matrix benchmarks
./build/alma-benchmark --csv-bench
./build/alma-benchmark --csv-bench --csv   # CSV output
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

## Low-Rank Optimization

The library automatically detects low-rank blocks using truncated SVD:

- Singular values > 10% of maximum are retained
- Only used when computational savings exceed threshold
- Ideal for matrices with structure (sparse, banded, low-rank)

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
