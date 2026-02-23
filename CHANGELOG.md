## Unreleased

### New Features

- **Expanded CLI with subcommands**: mul, add, scale, transpose, inv, det, norm, svd, qr, lu, solve
- **Comprehensive linear algebra API**: Added 15+ new functions for matrix operations
- **JSON output support**: All commands support `-j` flag for JSON output

### Build System Improvements

- **Static linking option**: `-Dstatic-link=true` for single-file executables
- **Automatic BLAS backend detection**: Detects MKL, OpenBLAS, BLIS, and Apple Accelerate
- **Platform-specific optimizations**: 
  - x86_64: AVX, AVX2, FMA optimizations
  - ARM64: ARMv8.2-a+dotprod optimizations
  - macOS: Accelerate framework support
- **Compiler optimizations**: -march=native, -ffast-math, -funroll-loops

### Performance

- **At parity with OpenBLAS**: Worst case equals OpenBLAS performance
- All tests pass with numerical accuracy maintained

### Notes

To achieve better performance:
- **Use Intel MKL** on Intel CPUs (requires MKL installation)
- **Use Apple Accelerate** on macOS (default)
- Recompile with platform-specific BLAS for best results

---

## v0.1.0 (Initial Release)

- Add basic unit test `tests/test_alma.cpp` and refactor core functions into `src/alma.(h|cpp)` so the test can link the implementation.
