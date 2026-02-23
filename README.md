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
brew install openblas
make
make test
make bench
```

## Files

| Path | Description |
|------|-------------|
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
make              # Build main binary
make test         # Run test suite
make bench        # Run benchmark
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
make bench

# Custom size and block
make bench n=2048 block=256

# Sweep over sizes and blocks
./bench/benchmark --sweep

# CSV matrix benchmarks
./bench/benchmark --csv-bench
./bench/benchmark --csv-bench --csv   # CSV output
```

### Generate Test Matrices

```bash
python3 bench/generate_matrices.py
```

## Test Suite

28 tests across 7 categories:

| Category | Tests |
|----------|-------|
| Basic sizes | 4x4, 8x8, 16x16, 32x32 |
| Edge cases | 1x1, 2x2, 8x8, 128x128 |
| Random data | 32x32, 64x64, 128x128 |
| Special matrices | 12 (identity, zeros, diagonal, etc.) |
| Large matrices | 256x256 |
| CSV data | 512x512 |

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

The Makefile uses `g++-15` by default. Override:

```bash
make CXX=g++ CXXFLAGS="-O3 -std=c++17 -fopenmp"
```

## Notes

- Small matrices (n <= 256) use single BLAS call
- Block size must divide matrix dimension
- Low-rank detection adds overhead but speeds up structured matrices
- See [docs/performance.md](docs/performance.md) for tuning tips

## Documentation

- [Architecture](docs/architecture.md) — System design
- [Algorithm](docs/paper.md) — Mathematical details
- [Performance](docs/performance.md) — Tuning guide
- [Contributing](docs/contributing.md) — Development

License: MIT
