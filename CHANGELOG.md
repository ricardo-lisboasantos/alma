## Unreleased

### New Features

- **Expanded CLI with subcommands**: mul, add, scale, transpose, inv, det, norm, svd, qr, lu, solve
- **Comprehensive linear algebra API**: Added 15+ new functions for matrix operations
- **JSON output support**: All commands support `-j` flag for JSON output
- **Multi-platform support**: Linux, macOS, Windows, Android, iOS

### Tests

- **41 tests** (was 28): Added tests for inv, det, norm, transpose, add, scale, lu, svd, qr, solve

### Build System

- **Static linking option**: `-Dstatic-link=true` for single-file executables

---

## v0.1.0 (Initial Release)

- Add basic unit test `tests/test_alma.cpp` and refactor core functions into `src/alma.(h|cpp)` so the test can link the implementation.
