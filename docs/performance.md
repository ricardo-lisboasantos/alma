# Performance Guide

## Benchmarking

Run benchmarks using the included tools:

```bash
# Default benchmark (1024x1024)
make bench

# Custom size and block size
make bench n=2048 block=128

# Sweep over sizes and blocks
./bench/benchmark --sweep

# CSV matrix benchmarks (multiple patterns and sizes)
./bench/benchmark --csv-bench
./bench/benchmark --csv-bench --csv   # CSV output

# Quick benchmark
make quick
```

## Threading & Hardware Configuration

The benchmark automatically detects your hardware and configures fair comparison between OpenBLAS and alma:

```bash
# Show system info (physical cores, RAM, L3 cache, recommended block size)
./bench/benchmark --sysinfo

# Use specific number of threads (default: physical cores)
./bench/benchmark -t 8

# Disable CPU affinity binding
./bench/benchmark --no-affinity

# Combine options
./bench/benchmark -t 8 -b 256 -v
```

### Threading Strategy

To ensure fair comparison, the benchmark prevents thread pool interference:

| When testing | BLAS threads | OpenMP threads | CPU affinity |
|-------------|--------------|-----------------|--------------|
| OpenBLAS | All cores | 1 | Enabled |
| alma | 1 | All cores | Enabled |

This ensures:
- OpenBLAS uses multithreaded BLAS calls
- alma uses OpenMP parallelism
- They don't compete for CPU resources

### System Detection

The benchmark detects:
- **Physical cores**: Used as default thread count
- **Logical cores**: Available for hyperthreading
- **Total RAM**: For reference
- **L3 cache**: Recommends optimal block size

### Command-Line Options

| Option | Description |
|--------|-------------|
| `-s N` | Matrix size (default: 1024) |
| `-b N` | Block size (default: auto from L3 cache) |
| `-r N` | Number of repeats (default: 3) |
| `-t N` | Number of threads (default: physical cores) |
| `--no-affinity` | Disable CPU affinity binding |
| `--sysinfo` | Print system info and exit |
| `-v` | Verbose output |

## Typical Results

### OpenBLAS vs alma (Apple Silicon M2)

| Matrix | Pattern | Block | OpenBLAS | alma | Speedup |
|--------|---------|-------|----------|------|---------|
| 512x512 | random | 128 | 3.90 ms | 2.05 ms | 1.90x |
| 1024x1024 | random | 128 | 16.00 ms | 11.45 ms | 1.40x |
| 1024x1024 | sparse | 128 | 17.31 ms | 11.93 ms | 1.45x |
| 1024x1024 | identity | 128 | 19.55 ms | 11.79 ms | 1.66x |
| 1024x1024 | banded | 128 | 16.62 ms | 11.74 ms | 1.42x |
| 2048x2048 | random | 256 | 138.57 ms | 88.69 ms | 1.56x |
| 2048x2048 | sparse | 256 | 102.36 ms | 95.34 ms | 1.07x |
| 2048x2048 | identity | 256 | 107.41 ms | 86.14 ms | 1.25x |
| 2048x2048 | banded | 256 | 103.64 ms | 85.51 ms | 1.21x |

Results vary based on hardware and system load.

## CSV Benchmark Matrices

Pre-generated matrices in `bench/data/`:

| Size | Patterns |
|------|----------|
| 512 | random |
| 1024 | random, sparse, identity, banded |
| 2048 | random, sparse, identity, banded |
| 4096 | random, sparse, identity |

Generate more:

```bash
python3 bench/generate_matrices.py
```

## Tuning Parameters

### Block Size

The block size controls the granularity of the algorithm:

| Block Size | Pros | Cons |
|------------|------|------|
| Small (32-64) | Better cache utilization | More overhead |
| Large (128-256) | Less overhead | Worse cache behavior |

**Recommendation**: The benchmark auto-detects optimal block size from L3 cache. For manual tuning, start with block size 64-128. Test 32, 64, 128, 256 for your hardware.

**Formula**: `block ≈ sqrt(L3_cache / (2 * sizeof(double)))`

For 8MB L3 cache: `sqrt(8MB / 16 bytes) ≈ 512`

### Cache-aware defaults & API

The library now detects the L3 cache at runtime and picks an optimal block size automatically. Call `alma_get_optimal_block_size()` from client code to retrieve the value used by `alma_multiply_auto()`.

Detection is platform-aware: on macOS it queries `hw.l3cachesize`, on Linux it reads `/sys/devices/system/cpu/.../index3/size`, and it falls back to an 8MB default when detection fails.

### Matrix Pattern Performance

- **Dense/random**: Best speedup (~1.5-2x)
- **Identity/diagonal**: Good speedup (~1.2-1.7x)
- **Sparse**: Lower speedup (~1.1-1.5x) due to memory access patterns

### Small Matrix Threshold

Matrices where n <= 256 use a single BLAS call:

```cpp
if (n <= 256) {
    cblas_dgemm(...);
    return;
}
```

### Low-Rank Threshold

Blocks with Frobenius norm < 1e-3 are classified as low-rank:

```cpp
static constexpr double LOWRANK_THRESHOLD = 1e-3;
```

### Rank Estimation (SVD heuristic)

An optional lightweight SVD-based rank estimator is used during block classification to decide whether a block can be treated as low-rank. The estimator is heuristic (cheap) — it inspects norms and diagonal contributions and returns a small integer rank estimate. When enabled, blocks with estimated rank significantly smaller than the block size are classified as `LowRank` and can enable low-rank multiplication paths.

The rank estimator is enabled by default in `alma_multiply_auto`/`alma_multiply` and can be toggled via the `use_svd` parameter in `alma_multiply_full`.

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

1. **Low GFLOPS**: Check thread configuration (`--sysinfo` to verify)
2. **Inconsistent results**: Verify matrix alignment
3. **Memory errors**: Ensure blockSize divides n evenly
4. **Slow for sparse**: Consider dense fallback for sparse matrices
5. **Thread contention**: Use `--no-affinity` if running alongside other workloads
