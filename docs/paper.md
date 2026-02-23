# Algorithm

## Blocked Matrix Multiplication

ALMA implements blocked matrix multiplication where the input matrices are divided into square blocks of configurable size. This approach provides better cache utilization compared to naive matrix multiplication.

Given matrices $A \in \mathbb{R}^{n \times n}$ and $B \in \mathbb{R}^{n \times n}$, partitioned into blocks:

$$
A = \begin{bmatrix} A_{11} & A_{12} \\ A_{21} & A_{22} \end{bmatrix}, \quad
B = \begin{bmatrix} B_{11} & B_{12} \\ B_{21} & B_{22} \end{bmatrix}
$$

The block-wise multiplication is:

$$
C_{ij} = \sum_{l} A_{il} B_{lj}
$$

## Block Classification with SVD

Each block is analyzed using Singular Value Decomposition (SVD) to determine if it exhibits low-rank structure, which enables specialized optimizations.

### Singular Value Decomposition

For a matrix $A \in \mathbb{R}^{m \times n}$ of rank $r$, the SVD is:

$$
A = U \Sigma V^T = \sum_{i=1}^{r} \sigma_i u_i v_i^T
$$

Where:
- $U = [u_1, ..., u_r]$ — left singular vectors ($m \times r$)
- $\Sigma = \text{diag}(\sigma_1, ..., \sigma_r)$ — singular values
- $V = [v_1, ..., v_r]$ — right singular vectors ($n \times r$)

### Rank Determination

The effective rank is determined by keeping singular values above a threshold:

$$
r = \max\{i : \sigma_i / \sigma_1 > \tau\}
$$

Where $\tau = 0.1$ (10% threshold).

### Classification Logic

```
if rank < blockSize and savings > threshold:
    type = LowRank
    store (U, S, VT) factorization
else:
    type = Dense
```

The threshold ensures low-rank representation only when computational savings justify the SVD overhead.

## Low-Rank Multiplication

For two low-rank blocks $A = U_a S_a V_a^T$ and $B = U_b S_b V_b^T$:

$$
C = A \cdot B = U_a (S_a V_a^T U_b S_b) V_b^T
$$

The computation proceeds:

1. $M = V_a^T U_b$ — $(r_a \times r_b)$ matrix
2. $M_{ij} \leftarrow M_{ij} \cdot S_{a,i} \cdot S_{b,j}$ — scale by singular values
3. $T = M V_b^T$ — $(r_a \times n)$ temporary
4. $C = U_a T$ — $(m \times n)$ result

This reduces complexity from $O(mnk)$ to $O(mr_a n + r_a r_b n + m r_a r_b)$.

## BLAS dgemm

Dense block multiplications use the BLAS `dgemm` routine:

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

### Phase 1: Block Classification

For each block, compute SVD and classify:

```
for each block (bi, bj):
    data = &A[bi * blockSize * n + bj * blockSize]
    (U, S, VT) = dgesvd(data, blockSize, blockSize)
    rank = count(S[i] / S[0] > 0.1)
    if rank < blockSize and compute_savings > 16 * blockSize:
        meta.type = LowRank
        meta.U, meta.S, meta.VT = stored factors
    else:
        meta.type = Dense
```

### Phase 2: Block Multiplication

```
for bi in 0..numBlocks:
    for bj in 0..numBlocks:
        C_block = 0
        for bk in 0..numBlocks:
            if A_block is LowRank and B_block is LowRank:
                C_block += lowrank_multiply(A_block, B_block)
            else:
                C_block += dgemm(A_block, B_block)
```

The first accumulation uses `alpha=0`, subsequent ones use `alpha=1`.

### Phase 3: Cleanup

Free allocated SVD factors:

```
for each block metadata:
    free(U); free(S); free(VT)
```

## Complexity Analysis

| Phase | Dense Operations | Low-Rank Operations |
|-------|-----------------|---------------------|
| Classification | $O(n^3 / b^2)$ SVD | Same |
| Multiplication | $O(n^3 / b)$ | $O(n^2 r)$ avg |

Where:
- $b$ — block size
- $r$ — average effective rank (typically $\ll b$ for structured matrices)

### Trade-offs

- **Small block size**: Better cache utilization, more overhead from SVD
- **Large block size**: Less overhead, worse cache behavior, higher SVD cost
- **Threshold (τ)**: Lower = more low-rank blocks, higher = more dense classifications
- **Savings threshold**: Minimum compute savings required to use low-rank form

## Parallelization

The algorithm uses OpenMP for parallel execution:

```cpp
#pragma omp parallel for collapse(2) schedule(dynamic, 1)
for (int bi = 0; bi < numBlocks; ++bi) {
    for (int bj = 0; bj < numBlocks; ++bj) {
        // Classify or multiply block
    }
}
```

The `collapse(2)` directive flattens the nested loops for better load balancing. The `schedule(dynamic, 1)` assigns one block pair per thread to handle irregular workloads from mixed dense/low-rank blocks.

## Numerical Stability

The implementation uses:
- `double` precision (15-16 significant digits)
- LAPACK dgesvd for numerically stable SVD
- BLAS for optimized kernels

For matrices with extreme scale differences, consider preprocessing (normalization) before multiplication.

## Thresholds

| Parameter | Value | Description |
|-----------|-------|-------------|
| `LOWRANK_RATIO_THRESHOLD` | 0.1 | Minimum singular value ratio |
| `LOWRANK_MIN_SAVINGS` | 16×blockSize | Minimum compute savings |

These thresholds balance SVD overhead against potential speedups from low-rank arithmetic.
