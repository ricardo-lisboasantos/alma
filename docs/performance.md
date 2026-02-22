# Performance Guide

## Benchmarking

Run benchmarks using the included tools:

```bash
# Quick benchmark with default parameters
make bench

# Custom size and block size
make bench n=2048 block=128

# Quick benchmark
make quickbench
```

## Typical Results

### OpenBLAS (Apple Silicon M2(8c/8g))

| Matrix Size | Block Size | Time (ms) | GFLOPS |
|-------------|------------|-----------|--------|
| 512 | 64 | ~15 | ~45 |
| 1024 | 64 | ~95 | ~55 |
| 2048 | 128 | ~650 | ~65 |

Results vary based on hardware and system load.

## Tuning Parameters

### Block Size

The block size controls the granularity of the algorithm:

| Block Size | Pros | Cons |
|------------|------|------|
| Small (32-64) | Better cache utilization | More overhead |
| Large (128-256) | Less overhead | Worse cache behavior |

**Recommendation**: Start with block size 64-128. For your hardware, test sizes 32, 64, 128, 256.

### Small Matrix Threshold

Matrices where n <= 256 use a single BLAS call:

```cpp
if (n <= 256) {
    cblas_dgemm(...);
    return;
}
```

This threshold can be adjusted in `alma.cpp` if needed.

### Low-Rank Threshold

Blocks with Frobenius norm < 1e-3 are classified as low-rank:

```cpp
static constexpr double LOWRANK_THRESHOLD = 1e-3;
```

Lower values classify more blocks as dense; higher values classify more as low-rank.

## Compiler Optimizations

The Makefile enables aggressive optimizations:

```makefile
CXXFLAGS = -O3 -std=c++17 -fopenmp -march=native -ffast-math
```

| Flag | Effect |
|------|--------|
| `-O3` | Full optimization |
| `-march=native` | CPU-specific optimizations |
| `-ffast-math` | Relaxed floating-point precision |
| `-fopenmp` | Parallel execution |


## Common Performance Issues

1. **Low GFLOPS**: Check OpenBLAS thread count
2. **Inconsistent results**: Verify matrix alignment
3. **Memory errors**: Ensure blockSize divides n evenly
