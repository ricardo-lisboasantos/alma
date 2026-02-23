# Low-Level Design

## Data Structures

### BlockType

```cpp
enum class BlockType { LowRank, Dense };
```

Enumeration distinguishing between dense blocks and blocks with low-rank structure.

### BlockMeta

```cpp
struct BlockMeta {
    BlockType type;    // Dense or LowRank classification
    int rows;           // Number of rows in block
    int cols;           // Number of columns in block
    int rank;           // Actual rank (0 for zero blocks, < blockSize for low-rank)
    int ld;             // Leading dimension (row stride in original matrix)
    double* U;          // Left singular vectors (low-rank only)
    double* S;          // Singular values (low-rank only)
    double* VT;         // Right singular vectors transposed (low-rank only)
};
```

Stores block metadata including SVD factorization for low-rank blocks.

### MatrixBlock

```cpp
struct MatrixBlock {
    double* data;       // Pointer to block data
    BlockMeta meta;     // Block metadata
};
```

Wrapper combining block data pointer with its metadata.

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

### TunedConfig

```cpp
struct TunedConfig {
    int blockSize;
    bool useOpenBLAS;
    bool initialized;
};
```

Runtime configuration determined by auto-tuning.

## Core Functions

### classify_block

```cpp
BlockMeta classify_block(double* data, int rows, int cols, int ld);
```

Classifies a single block using SVD:

1. Computes Frobenius norm; if near-zero, marks as LowRank with rank 0
2. Calls `compute_block_meta()` with `compute_svd=true`
3. Performs SVD via LAPACK `dgesvd`
4. Computes effective rank using `compute_rank_from_singular_values()`
5. If rank < rows × 0.25, stores U, S, VT factors; otherwise marks as Dense

### compute_block_meta

```cpp
static BlockMeta compute_block_meta(double* data, int rows, int cols, int ld, bool compute_svd);
```

Internal function handling SVD computation and block classification.

### compute_rank_from_singular_values

```cpp
static int compute_rank_from_singular_values(double* S, int n, double threshold);
```

Determines rank by counting singular values exceeding threshold ratio:
```cpp
if (S[i] / max_sv > threshold) rank = i + 1;
```

### multiply_dense_block

```cpp
void multiply_dense_block(const MatrixBlock& A, const MatrixBlock& B, MatrixBlock& C);
```

Wrapper around BLAS dgemm:
```cpp
cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
            A.meta.rows, B.meta.cols, A.meta.cols,
            1.0, A.data, A.meta.ld, B.data, B.meta.ld,
            1.0, C.data, C.meta.ld);
```

### multiply_lowrank_block

```cpp
void multiply_lowrank_block(const MatrixBlock& A, const MatrixBlock& B, MatrixBlock& C);
```

Multiplies two low-rank blocks using SVD factorization:

Given A = U_a × S_a × V_a^T and B = U_b × S_b × V_b^T:

1. **M = V_a^T × U_b**: Compute intermediate (ra × rb) matrix
2. **Scale by singular values**: M[i,j] *= S_a[i] × S_b[j]
3. **T = M × V_b^T**: Compute (ra × n) temporary
4. **C = U_a × T**: Final result accumulated into C.data

Complexities: O(ra × rb × k) + O(ra × n × rb) + O(m × n × ra) vs O(m × n × k) for dense

### alma_multiply_block

```cpp
void alma_multiply_block(const MatrixBlock& A, const MatrixBlock& B, MatrixBlock& C);
```

Dispatches to appropriate multiplication based on block types:
- Both LowRank → `multiply_lowrank_block()`
- Otherwise → `multiply_dense_block()`

### free_block_meta

```cpp
void free_block_meta(BlockMeta& meta);
```

Frees allocated SVD factors (U, S, VT) to prevent memory leaks.

## Main Multiplication Flow

### alma_multiply_full

```cpp
AlmaError alma_multiply_full(double* A, double* B, double* C, int n, int blockSize);
```

Full blocked multiplication algorithm:

1. **Validation**: Check pointers, dimensions, blockSize validity
2. **Small matrix fallback**: If n ≤ 256 or numBlocks ≤ 1 or blockSize ≥ 512, use single dgemm
3. **Classification phase** (current implementation uses dense dgemm):
   - Allocate metadata arrays for A and B blocks
   - Classify each block using SVD
4. **Multiplication phase**:
   - Parallel loop over output blocks (bi, bj)
   - For each output block, accumulate contributions from all k blocks
   - Use alpha=0 for first accumulation, alpha=1 for subsequent
5. **Cleanup**: Free metadata arrays

### alma_multiply

```cpp
AlmaError alma_multiply(double* A, double* B, double* C, int n, int blockSize);
```

Entry point that validates inputs and delegates to `alma_multiply_full()`.

### alma_multiply_auto

```cpp
AlmaError alma_multiply_auto(double* A, double* B, double* C, int n);
```

Automatic block size selection:

1. Lazy initialization via double-checked locking
2. Calls `tune_configuration()` to benchmark different block sizes
3. Falls back to OpenBLAS if ALMA is slower
4. Uses tuned blockSize or defaults to 128

## Auto-Tuning

### tune_configuration

```cpp
static void tune_configuration();
```

Automatically selects optimal configuration:

1. Tests sizes: {512, 1024}
2. Tests blocks: {64, 128, 256}
3. Benchmarks OpenBLAS dgemm as baseline
4. Compares ALMA performance for each block size
5. Sets `useOpenBLAS=true` if ALMA is slower, otherwise uses best blockSize

### benchmark_config

```cpp
static double benchmark_config(double* A, double* B, double* C, int n, int blockSize, int repeats);
```

Runs multiple iterations and returns minimum execution time.

### get_cache_optimal_block_size

```cpp
static int get_cache_optimal_block_size();
```

Computes block size from L3 cache:
```cpp
return sqrt(cache / (2.0 * sizeof(double)));
```

### get_l3_cache_size

```cpp
static size_t get_l3_cache_size();
```

Platform-specific L3 cache detection:
- macOS: `sysctlbyname("hw.l3cachesize")`
- Linux: Read `/sys/devices/system/cpu/cpu0/cache/index3/size`

## Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `LOWRANK_RATIO_THRESHOLD` | 0.1 | Minimum singular value ratio |
| `LOWRANK_MIN_SAVINGS` | 16 | Minimum compute savings multiplier |
| `LOWRANK_MAX_RATIO` | 0.25 | Maximum rank ratio for low-rank classification |
| `TUNING_MAX_TIME_MS` | 500 | Maximum tuning time |
| `TUNING_MIN_REPEATS` | 2 | Minimum benchmark iterations |

## Memory Layout

Matrices use row-major layout compatible with BLAS:
```
A[i][j] = A[i * ld + j]
```

The leading dimension (`ld`) enables submatrix operations within larger matrices.

## Error Handling

| Error | Condition |
|-------|-----------|
| `NullPointer` | A, B, or C is NULL |
| `InvalidDimension` | n <= 0 |
| `InvalidBlockSize` | blockSize <= 0, doesn't divide n, or blockSize > n |

`alma_error_string()` converts error codes to human-readable messages.

## Thread Safety

- `tune_configuration()` uses double-checked locking with `std::mutex`
- Global `g_tuned_config` is atomic-safe via mutex protection
- Block multiplication is thread-safe due to OpenMP parallelization
