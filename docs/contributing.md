# Contributing

## Development Setup

### Prerequisites

- C++17 compiler (GCC 10+, Clang 12+, Apple clang 15+)
- OpenBLAS or other BLAS library
- OpenMP (libomp on macOS)
- Meson + Ninja

### macOS

```bash
brew install openblas meson ninja
meson setup build
meson compile -C build
```

### Linux (Debian/Ubuntu)

```bash
sudo apt install meson ninja-build libopenblas-dev libomp-dev
meson setup build
meson compile -C build
```

## Building

```bash
# Configure (first time or after clean)
meson setup build

# Compile
meson compile -C build

# Rebuild after changes
meson compile -C build

# Clean build
meson setup build --wipe
```

## Running Tests

```bash
meson test -C build
```

All tests must pass before submitting changes.

```text
Test: Small matrix (4x4, block=2)... PASSED
Test: Medium matrix (64x64, block=16)... PASSED
...
==================
Results: 1/1 alma:alma OK
```

## Running Benchmarks

```bash
# Default benchmark
./build/alma-benchmark -s 1024 -b 128 -r 3

# Custom parameters
./build/alma-benchmark -s 2048 -b 128 -r 3

# Quick benchmark
./build/quick_bench
```

### Linux (Debian/Ubuntu)

```bash
sudo apt install meson ninja-build libopenblas-dev libomp-dev
meson setup build
meson compile -C build
```

## Code Style

- C++17 with standard library
- Use `static` for internal (non-API) functions
- Avoid comments unless explaining non-obvious logic
- Keep functions focused and small
- Use DRY principles — extract common logic into helpers

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Functions | snake_case | `alma_multiply` |
| Structs | PascalCase | `BlockMeta` |
| Enums | PascalCase | `BlockType` |
| Constants | SCREAMING_SNAKE_CASE | `LOWRANK_THRESHOLD` |

### Example Function

```cpp
static void classify_all_blocks(double* A, BlockMeta* meta, int n, int blockSize) {
    int numBlocks = n / blockSize;
    
    #pragma omp parallel for schedule(static)
    for (int bi = 0; bi < numBlocks; ++bi) {
        for (int bj = 0; bj < numBlocks; ++bj) {
            // Implementation
        }
    }
}
```

## Adding Tests

Tests are in `tests/test_alma.cpp`. Add new tests following the existing pattern:

```cpp
void test_name() {
    // Setup
    int n = 64;
    int blockSize = 16;
    std::vector<double> A(n * n), B(n * n), C(n * n);
    
    // Initialize
    // ...
    
    // Execute
    alma_multiply(A.data(), B.data(), C.data(), n, blockSize);
    
    // Verify
    // ...
}
```

## Project Structure

```
alma/
├── src/
│   ├── alma.h       # Public API
│   ├── alma.cpp      # Implementation
│   └── main.cpp      # Example
├── tests/
│   └── test_alma.cpp # Test suite
├── bench/
│   ├── benchmark.cpp  # Full benchmark
│   └── quick_bench.cpp # Quick benchmark
├── docs/
│   ├── architecture.md
│   ├── paper.md
│   ├── performance.md
│   └── contributing.md
├── meson.build
└── README.md
```

## Reporting Issues

Include:
1. Compiler version (`g++ --version`)
2. OS and architecture
3. BLAS library version
4. Minimal reproduction case

## Pull Requests

1. Fork the repository
2. Create a feature branch
3. Make changes with tests
4. Ensure `meson test -C build` passes
5. Submit PR with description
