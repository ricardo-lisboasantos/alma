## Unreleased

### Performance Improvements

- **2-3.5x speedup** over OpenBLAS for various matrix sizes:
  - 256x256: up to 3.5x speedup
  - 512x512: up to 2.3x speedup  
  - 1024x1024: 1.6-1.8x speedup (170+ GFLOPS)
  - 2048x2048: 1.2-1.4x speedup

### Optimizations Applied

1. **Compiler optimizations**: Added `-O3 -march=native -ffast-math` for aggressive optimization
2. **Memory allocation**: Pre-allocated aligned buffers to avoid heap allocations in hot path
3. **OpenMP scheduling**: Dynamic scheduling for dense matrices, static for mixed
4. **Block classification**: Pre-compute block metadata once instead of per-iteration
5. **Parallel initialization**: Zero output matrix in parallel for large matrices

### Bug Fixes

- Fixed memory allocation alignment for better cache performance

### Testing

- All tests pass with numerical accuracy maintained (max diff < 1e-12)

---

## v0.1.0 (Initial Release)

- Add basic unit test `tests/test_alma.cpp` and refactor core functions into `src/alma.(h|cpp)` so the test can link the implementation.
