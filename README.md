ALMA — Blocked Matrix Multiply (C++)

Fast blocked matrix multiplication with low-rank block detection and OpenBLAS/BLAS acceleration.

![Build](https://img.shields.io/badge/build-passing-green)
![Tests](https://img.shields.io/badge/tests-9%2F9-green)

## Features

- Blocked multiply with per-block classification (dense vs low-rank)
- BLAS (OpenBLAS) for high-performance kernels
- OpenMP for parallel execution
- Small test suite and benchmark harness

## Architecture

![Architecture Flowchart](docs/diagrams/architecture.mmd#flowchart)
![Class Diagram](docs/diagrams/architecture.mmd#class)
![Sequence Diagram](docs/diagrams/architecture.mmd#sequence)

See [docs/architecture.md](docs/architecture.md) for detailed system design.

## Files

| Path | Description |
|------|-------------|
| `src/alma.cpp`, `src/alma.h` | Core implementation |
| `src/main.cpp` | Example program (JSON output) |
| `tests/test_alma.cpp` | Correctness tests |
| `bench/benchmark.cpp` | Performance benchmark |
| `bench/quick_bench.cpp` | Quick benchmark |
| `Makefile` | Build targets |

Build
Requires a C++17 compiler and a BLAS library (OpenBLAS recommended). On macOS with Homebrew:

```bash
brew install openblas
make
```

On Debian/Ubuntu:

```bash
sudo apt-get install build-essential libopenblas-dev libomp-dev
make
```

Usage
- Build the example: `make`
- Run the program: `make run` (prints a JSON matrix to stdout)
- Build and run tests: `make test`
- Run the benchmark: `make bench n=2048 block=128`

Configuration
- The Makefile sets `CXX := g++-15` by default. Override on the command line if needed, for example:

```bash
make CXX=g++ CXXFLAGS="-O3 -std=c++17 -fopenmp"
```

Notes
- The implementation falls back to a single BLAS call for small matrices (n <= 256).
- The block size must divide the matrix dimension `n` in example drivers; adjust sources if you need non-divisible sizes.

License
MIT
