# Architecture

## Overview

ALMA implements a blocked matrix multiplication algorithm with per-block classification to optimize for matrices with low-rank structure. The library provides both low-level block operations and high-level matrix multiplication entry points.

![Flowchart](diagrams/architecture.mmd#flowchart)

## System Design

The system consists of three main layers:

1. **API Layer** — Public functions in `alma.h`
2. **Classification Layer** — Block analysis and categorization
3. **Execution Layer** — BLAS-powered multiplication with OpenMP parallelism

## Data Structures

![Class Diagram](diagrams/architecture.mmd#class)

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
| `rank_est` | int | Estimated rank (1 for low-rank, blockSize for dense) |
| `ld` | int | Leading dimension (row stride in original matrix) |

### MatrixBlock

| Field | Type | Description |
|-------|------|-------------|
| `data` | double* | Pointer to block data |
| `meta` | BlockMeta | Block metadata |

## API Reference

### alma_multiply

```cpp
void alma_multiply(double* A, double* B, double* C,
                   int n, int blockSize);
```

Main entry point for blocked matrix multiplication.

**Parameters:**
- `A` — Input matrix A (mxk)
- `B` — Input matrix B (kxn)
- `C` — Output matrix C (mxn), must be pre-allocated
- `n` — Matrix dimension (assumes square matrices)
- `blockSize` — Size of each block

**Behavior:**
- For n <= 256: Falls back to single BLAS dgemm
- For n > 256: Uses blocked algorithm with parallel execution

### classify_block

```cpp
BlockMeta classify_block(double* data, int rows, int cols);
```

Classify a single block as Dense or LowRank.

**Parameters:**
- `data` — Pointer to block data
- `rows` — Number of rows
- `cols` — Number of columns

**Returns:** BlockMeta with classification

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

Multiply two low-rank blocks. Currently delegates to dense multiplication.

### alma_multiply_block

```cpp
void alma_multiply_block(const MatrixBlock& A,
                         const MatrixBlock& B,
                         MatrixBlock& C);
```

Dispatch multiplication based on block types.

## Execution Model

![Sequence Diagram](diagrams/architecture.mmd#sequence)

## Key Optimizations

| Optimization | Description | Impact |
|--------------|-------------|--------|
| Small matrix fallback | Single BLAS call for n <= 256 | Avoids overhead |
| Block classification | Identifies low-rank blocks | Enables optimizations |
| Parallel execution | OpenMP collapse(2) | Scales with cores |
| Cache-aligned allocation | 64-byte aligned metadata | Reduces cache misses |

## Memory Layout

Matrices use row-major layout compatible with BLAS:

```
A[i][j] = A[i * ld + j]
```

The leading dimension (`ld`) allows for submatrix operations within larger matrices.
