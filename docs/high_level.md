# High-Level Design

## Overview

ALMA (Adaptive Low-rank Matrix Algebra) is a C++ library implementing blocked matrix multiplication with intelligent low-rank detection and optimization. The library automatically identifies low-rank structure in matrix blocks and applies specialized fast multiplication algorithms.

## System Architecture

```mermaid
flowchart TD
    A[API Layer (alma.h)<br/>alma_multiply<br/>alma_multiply_full<br/>alma_multiply_auto]
    B[Classification Layer<br/>SVD-based block analysis and categorization<br/>BlockType::LowRank / BlockType::Dense]
    C[Execution Layer<br/>BLAS dgemm (dense)<br/>SVD-based multiply (low-rank)<br/>OpenMP Parallelism]

    A --> B
    B --> C
```

## Components

### API Layer

The public interface provides three multiplication entry points:

| Function | Purpose |
|----------|---------|
| `alma_multiply()` | Main entry point, delegates to `alma_multiply_full()` |
| `alma_multiply_full()` | Full blocked algorithm with classification |
| `alma_multiply_auto()` | Automatic block size selection based on L3 cache |

### Classification Layer

Analyzes each matrix block to determine if it exhibits low-rank structure:

1. **SVD Computation**: Uses LAPACK `dgesvd` to compute singular values/vectors
2. **Rank Detection**: Keeps singular values > 10% of maximum
3. **Cost-Benefit**: Only uses low-rank if savings exceed threshold

### Execution Layer

Dispatches to appropriate multiplication routine:
- **Dense blocks**: BLAS `dgemm`
- **Low-rank blocks**: Custom SVD-based multiplication

## Data Flow

```mermaid
flowchart TD
    A[Input Matrices (A, B)]
    B[Block Partition<br/>Divide into blocks of size blockSize × blockSize]
    C[Classify Block (A, B)<br/>For each block: compute SVD, determine type]
    D[Block Multiply<br/>Parallel execution with OpenMP<br/>C = A × B<br/>Dispatch based on block types]
    E[Output Matrix (C)]

    A --> B
    B --> C
    C --> D
    D --> E
```

## Key Design Decisions

### 1. Block Size Selection

- **Manual**: User specifies `blockSize` parameter
- **Automatic**: `alma_multiply_auto()` uses L3 cache to compute optimal size

### 2. Small Matrix Optimization

For n ≤ 256, falls back to single BLAS call to avoid classification overhead.

### 3. Low-Rank Detection Threshold

- Ratio threshold: 0.1 (keep singular values > 10% of max)
- Savings threshold: 16× blockSize minimum compute savings required

### 4. Parallelization Strategy

- OpenMP `collapse(2)` with `schedule(dynamic, 1)` for load balancing
- Fine-grained block scheduling handles irregular workloads

## Dependencies

| Library | Purpose |
|---------|---------|
| OpenBLAS/LAPACK | SVD computation, BLAS dgemm |
| OpenMP | Parallel execution |

## Configuration

Static configuration managed via `TunedConfig`:
- `blockSize`: Optimal block size for current system
- `useOpenBLAS`: Whether to use fallback BLAS
- `initialized`: Lazy initialization flag
