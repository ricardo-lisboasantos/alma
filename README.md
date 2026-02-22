ALMA — Blocked Matrix Multiply (C++)

Fast blocked matrix multiplication example with simple low-rank block detection and OpenBLAS/BLAS acceleration.

Features
- Blocked multiply with per-block classification (dense vs low-rank)
- Uses BLAS (OpenBLAS) for high-performance kernels and OpenMP for threading
- Small test and simple benchmark harness included

Files
- `src/alma.cpp`, `src/alma.h` — core implementation
- `src/main.cpp` — example program that prints the result as JSON
- `tests/test_alma.cpp` — small correctness test
- `bench/benchmark.cpp`, `bench/quick_bench.cpp` — benchmark drivers
- `Makefile` — convenient build and run targets

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
