# Architecture

## Overview

ALMA implements a blocked matrix multiplication algorithm with per-block classification to optimize for matrices with low-rank structure. The library provides both low-level block operations and high-level matrix multiplication entry points.

![Flowchart](diagrams/architecture.mmd)

## System Design

The system consists of three main layers:

1. **API Layer** — Public functions in `alma.h`
2. **Classification Layer** — SVD-based block analysis and categorization
3. **Execution Layer** — BLAS-powered and low-rank multiplication with OpenMP parallelism

## Data Structures

![Class Diagram](diagrams/architecture-classdiagram.mmd)

### BlockType

```cpp
enum class BlockType { LowRank, Dense };
```

Enumeration distinguishing between dense blocks and blocks with low-rank structure.

### BlockMeta

| Field | Type | Description |
|-------|------|-------------|
| `type` | BlockType | Dense or LowRank classification |
| `rows` | int | Number of rows in block |
| `cols` | int | Number of columns in block |
| `rank` | int | Actual rank (0 for zero blocks, < blockSize for low-rank) |
| `ld` | int | Leading dimension (row stride in original matrix) |
| `U` | double* | Left singular vectors (low-rank only) |
| `S` | double* | Singular values (low-rank only) |
| `VT` | double* | Right singular vectors transposed (low-rank only) |

### MatrixBlock

| Field | Type | Description |
|-------|------|-------------|
| `data` | double* | Pointer to block data |
| `meta` | BlockMeta | Block metadata |

### AlmaError

```cpp
enum class AlmaError {
    Success = 0,
    NullPointer,
    InvalidDimension,
    InvalidBlockSize,
    DimensionMismatch
};
```

Error codes returned by API functions.

## API Reference

### alma_multiply

```cpp
AlmaError alma_multiply(double* A, double* B, double* C,
                       int n, int blockSize);
```

Main entry point for blocked matrix multiplication.

**Parameters:**
- `A` — Input matrix A (n×n)
- `B` — Input matrix B (n×n)
- `C` — Output matrix C (n×n), must be pre-allocated
- `n` — Matrix dimension (square matrices only)
- `blockSize` — Size of each block (must divide n)

**Returns:** `AlmaError` indicating success or failure type

**Behavior:**
- For n <= 256: Falls back to single BLAS dgemm
- For n > 256: Uses blocked algorithm with SVD-based classification and parallel execution

### alma_multiply_full

```cpp
AlmaError alma_multiply_full(double* A, double* B, double* C,
                             int n, int blockSize);
```

Low-level blocked multiplication with full classification and low-rank optimization.

### alma_multiply_auto

```cpp
AlmaError alma_multiply_auto(double* A, double* B, double* C, int n);
```

Automatically selects optimal block size based on L3 cache and performs blocked multiplication.

### classify_block

```cpp
BlockMeta classify_block(double* data, int rows, int cols, int ld);
```

Classify a single block as Dense or LowRank using SVD.

**Parameters:**
- `data` — Pointer to block data
- `rows` — Number of rows
- `cols` — Number of columns
- `ld` — Leading dimension

**Returns:** BlockMeta with classification and SVD factors (if low-rank)

### free_block_meta

```cpp
void free_block_meta(BlockMeta& meta);
```

Free allocated SVD factors (U, S, VT) in a BlockMeta structure.

### multiply_dense_block

```cpp
void multiply_dense_block(const MatrixBlock& A,
                          const MatrixBlock& B,
                          MatrixBlock& C);
```

Multiply two dense blocks using BLAS dgemm.

### multiply_lowrank_block

```cpp
void multiply_lowrank_block(const MatrixBlock& A,
                            const MatrixBlock& B,
                            MatrixBlock& C);
```

Multiply two low-rank blocks using SVD-based multiplication:
- Uses the formula: C = A * B = (U_a * S_a * VTa) * (U_b * S_b * VTb)
- Computes M = VTa * U_b, scales by singular values, then multiplies

### alma_multiply_block

```cpp
void alma_multiply_block(const MatrixBlock& A,
                         const MatrixBlock& B,
                         MatrixBlock& C);
```

Dispatch multiplication based on block types (low-rank or dense).

### alma_error_string

```cpp
const char* alma_error_string(AlmaError err);
```

Convert error code to human-readable string.

## Low-Rank Detection

The implementation uses truncated SVD to detect low-rank structure:

1. **SVD Computation**: Uses LAPACK `dgesvd` to compute singular values and vectors
2. **Rank Determination**: Keeps singular values > 10% of the maximum
3. **Cost-Benefit Analysis**: Only uses low-rank form if memory/compute savings exceed threshold

### Thresholds

| Parameter | Value | Description |
|-----------|-------|-------------|
| `LOWRANK_RATIO_THRESHOLD` | 0.1 | Minimum singular value ratio to keep |
| `LOWRANK_MIN_SAVINGS` | 16 | Minimum compute savings required |

## Execution Model

![Sequence Diagram](diagrams/architecture-sequence.mmd)

## Key Optimizations

| Optimization | Description | Impact |
|--------------|-------------|--------|
| Small matrix fallback | Single BLAS call for n <= 256 | Avoids overhead |
| SVD-based classification | Identifies low-rank blocks using LAPACK | Enables ~O(n²) multiplication |
| Low-rank multiplication | Uses U*S*VT factorization | Reduced complexity for structured matrices |
| Parallel execution | OpenMP collapse(2) | Scales with cores |
| Dynamic scheduling | Fine-grained block scheduling | Load balancing |

## Memory Layout

Matrices use row-major layout compatible with BLAS:

```
A[i][j] = A[i * ld + j]
```

The leading dimension (`ld`) allows for submatrix operations within larger matrices.

## Error Handling

All public API functions return `AlmaError`:

- `Success` — Operation completed successfully
- `NullPointer` — A NULL pointer was provided
- `InvalidDimension` — n <= 0
- `InvalidBlockSize` — blockSize <= 0 or doesn't divide n
- `DimensionMismatch` — Non-square matrices (future)

Use `alma_error_string()` to get human-readable error messages.
