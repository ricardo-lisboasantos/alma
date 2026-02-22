# Algorithm

## Blocked Matrix Multiplication

ALMA implements blocked matrix multiplication where the input matrices are divided into square blocks of configurable size. This approach provides better cache utilization compared to naive matrix multiplication.

Given matrices $A \in \mathbb{R}^{m \times k}$ and $B \in \mathbb{R}^{k \times n}$, partitioned into blocks:

$$
A = \begin{bmatrix} A_{11} & A_{12} \\ A_{21} & A_{22} \end{bmatrix}, \quad
B = \begin{bmatrix} B_{11} & B_{12} \\ B_{21} & B_{22} \end{bmatrix}
$$

The block-wise multiplication is:

$$
C_{ij} = \sum_{l} A_{il} B_{lj}
$$

## Block Classification

Each block is analyzed to determine if it exhibits low-rank structure, which could enable specialized optimizations.

### Frobenius Norm

The Frobenius norm is computed for each block:

$$
\|A\|_F = \sqrt{\sum_{i=1}^{m}\sum_{j=1}^{n} |a_{ij}|^2}
$$

### Classification Logic

```
if ||A||_F < 1e-3:
    type = LowRank
    rank_est = 1
else:
    type = Dense
    rank_est = blockSize
```

The threshold $10^{-3}$ was chosen empirically based on typical numerical precision requirements.

## BLAS dgemm

All block multiplications use the BLAS `dgemm` routine:

```c
cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
            M, N, K, 1.0, A, lda, B, ldb, 1.0, C, ldc);
```

Parameters:
- `M` — Number of rows of C
- `N` — Number of columns of C
- `K` — Number of columns of A / rows of B
- `lda`, `ldb`, `ldc` — Leading dimensions

## Algorithm Phases

### Phase 1: Classification

For each block, compute Frobenius norm and classify:

```
for each block (bi, bj):
    data = &A[bi * blockSize * n + bj * blockSize]
    norm = sqrt(sum(data[i]^2))
    if norm < threshold:
        meta.type = LowRank
    else:
        meta.type = Dense
```

### Phase 2: Initialization

Zero the output matrix:

```
for i in 0..n*n:
    C[i] = 0.0
```

### Phase 3: Block Multiplication

```
for bi in 0..numBlocks:
    for bj in 0..numBlocks:
        for bk in 0..numBlocks:
            C_block += A_block * B_block
```

The first accumulation uses alpha=0, subsequent ones use alpha=1.

## Complexity Analysis

| Phase | Operations | Memory |
|-------|------------|--------|
| Classification | $O(n^2)$ | $O(n^2)$ metadata |
| Initialization | $O(n^2)$ | $O(n^2)$ output |
| Multiplication | $O(n^3 / b)$ | $O(n^2)$ |

Where $b$ is the block size.

### Trade-offs

- **Small block size**: Better cache utilization, more overhead
- **Large block size**: Less overhead, potentially worse cache behavior
- **Threshold**: Lower = more dense blocks, higher = more low-rank classifications

## Parallelization

The algorithm uses OpenMP for parallel execution:

```cpp
#pragma omp parallel for collapse(2) schedule(dynamic, 1)
for (int bi = 0; bi < numBlocks; ++bi) {
    for (int bj = 0; bj < numBlocks; ++bj) {
        // ...
    }
}
```

The `collapse(2)` directive flattens the nested loops for better load balancing. The `schedule(dynamic, 1)` assigns one block pair per thread to handle irregular workloads.

## Numerical Stability

The implementation uses:
- `double` precision (15-16 significant digits)
- BLAS for optimized kernels
- No explicit error correction within blocks

For matrices with extreme scale differences, consider preprocessing (normalization) before multiplication.
