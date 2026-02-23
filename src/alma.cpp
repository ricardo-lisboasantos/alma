#include "alma.h"
#include <cmath>
#include <cblas.h>
#include <lapacke.h>
#include <omp.h>
#include <cstdlib>
#include <algorithm>
#include <sys/sysctl.h>
#include <chrono>
#if defined(__linux__)
#include <fstream>
#endif
#include <cstring>
#include <vector>
#include <mutex>

static constexpr double LOWRANK_RATIO_THRESHOLD = 0.1;
static constexpr double LOWRANK_MAX_RATIO = 0.25;

struct TunedConfig {
    int blockSize;
    bool useOpenBLAS;
    bool initialized;
};

static TunedConfig g_tuned_config = {0, false, false};
static std::mutex g_tuning_mutex;

const char* alma_error_string(AlmaError err) {
    switch (err) {
        case AlmaError::Success: return "Success";
        case AlmaError::NullPointer: return "Null pointer provided";
        case AlmaError::InvalidDimension: return "Invalid matrix dimension";
        case AlmaError::InvalidBlockSize: return "Invalid block size";
        case AlmaError::DimensionMismatch: return "Dimension mismatch (non-square matrix)";
    }
    return "Unknown error";
}

static size_t get_l3_cache_size() {
#if defined(__APPLE__)
    int64_t l3size = 0;
    size_t len = sizeof(l3size);
    sysctlbyname("hw.l3cachesize", &l3size, &len, NULL, 0);
    return static_cast<size_t>(l3size);
#elif defined(__linux__)
    std::ifstream caches("/sys/devices/system/cpu/cpu0/cache/index3/size");
    std::string line;
    if (std::getline(caches, line)) {
        size_t cache = std::stoull(line.substr(0, line.size() - 1));
        return (line.back() == 'K') ? cache * 1024 : cache * 1024 * 1024;
    }
#endif
    return 8UL * 1024 * 1024;
}

static int get_cache_optimal_block_size() {
    size_t cache = get_l3_cache_size();
    return static_cast<int>(std::sqrt(cache / (2.0 * sizeof(double))));
}

void free_block_meta(BlockMeta& meta) {
    if (meta.U) {
        free(meta.U);
        meta.U = nullptr;
    }
    if (meta.S) {
        free(meta.S);
        meta.S = nullptr;
    }
    if (meta.VT) {
        free(meta.VT);
        meta.VT = nullptr;
    }
}

static int compute_rank_from_singular_values(double* S, int n, double threshold) {
    if (n == 0) return 0;
    double max_sv = S[0];
    if (max_sv == 0) return 0;
    
    int rank = 0;
    for (int i = 0; i < n; ++i) {
        if (S[i] / max_sv > threshold) {
            rank = i + 1;
        }
    }
    return rank;
}

static BlockMeta compute_block_meta(double* data, int rows, int cols, int ld, bool compute_svd) {
    BlockMeta m;
    m.rows = rows;
    m.cols = cols;
    m.U = nullptr;
    m.S = nullptr;
    m.VT = nullptr;

    if (rows == 0 || cols == 0) {
        m.type = BlockType::Dense;
        m.rank = 0;
        return m;
    }

    double norm = 0.0;
    for (int i = 0; i < rows * cols; ++i) norm += data[i] * data[i];
    norm = std::sqrt(norm);

    if (norm < 1e-10) {
        m.type = BlockType::LowRank;
        m.rank = 0;
        return m;
    }

    if (!compute_svd) {
        m.type = BlockType::Dense;
        m.rank = rows;
        return m;
    }

    int min_dim = std::min(rows, cols);
    double* U = (double*)malloc(rows * min_dim * sizeof(double));
    double* S = (double*)malloc(min_dim * sizeof(double));
    double* VT = (double*)malloc(min_dim * cols * sizeof(double));
    double* superb = (double*)malloc(min_dim * sizeof(double));
    
    if (!U || !S || !VT || !superb) {
        free(U);
        free(S);
        free(VT);
        free(superb);
        m.type = BlockType::Dense;
        m.rank = rows;
        return m;
    }

    int lda = cols;
    int ldu = min_dim;
    int ldvt = cols;
    
    std::vector<double> data_copy(rows * cols);
    for (int i = 0; i < rows * cols; ++i) {
        data_copy[i] = data[i];
    }
    
    int info = LAPACKE_dgesvd(LAPACK_ROW_MAJOR, 'S', 'S', rows, cols, data_copy.data(), lda, 
                              S, U, ldu, VT, ldvt, superb);
    
    if (info != 0) {
        free(U);
        free(S);
        free(VT);
        free(superb);
        m.type = BlockType::Dense;
        m.rank = rows;
        return m;
    }

    m.rank = compute_rank_from_singular_values(S, min_dim, LOWRANK_RATIO_THRESHOLD);
    
    int effective_rank = m.rank;
    
    if (effective_rank > 0 && effective_rank < rows * LOWRANK_MAX_RATIO) {
        m.type = BlockType::LowRank;
        m.U = (double*)malloc(rows * effective_rank * sizeof(double));
        m.S = (double*)malloc(effective_rank * sizeof(double));
        m.VT = (double*)malloc(effective_rank * cols * sizeof(double));
        
        if (m.U && m.S && m.VT) {
            for (int i = 0; i < rows * effective_rank; ++i) {
                m.U[i] = U[i];
            }
            for (int i = 0; i < effective_rank; ++i) {
                m.S[i] = S[i];
            }
            for (int i = 0; i < effective_rank * cols; ++i) {
                m.VT[i] = VT[i];
            }
        } else {
            free(m.U);
            free(m.S);
            free(m.VT);
            m.U = nullptr;
            m.S = nullptr;
            m.VT = nullptr;
            m.type = BlockType::Dense;
            m.rank = rows;
        }
    } else {
        m.type = BlockType::Dense;
        m.rank = rows;
    }
    
    free(U);
    free(S);
    free(VT);
    free(superb);
    
    return m;
}

BlockMeta classify_block(double* data, int rows, int cols, int ld) {
    return compute_block_meta(data, rows, cols, ld, true);
}

AlmaError alma_multiply_full(double* A, double* B, double* C,
                             int n, int blockSize) {
    if (!A || !B || !C) {
        return AlmaError::NullPointer;
    }
    if (n <= 0) {
        return AlmaError::InvalidDimension;
    }
    if (blockSize <= 0 || n % blockSize != 0 || blockSize > n) {
        return AlmaError::InvalidBlockSize;
    }

    int numBlocks = n / blockSize;

    if (n <= 256 || numBlocks <= 1 || blockSize >= 512) {
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    n, n, n, 1.0, A, n, B, n, 0.0, C, n);
        return AlmaError::Success;
    }

    BlockMeta* A_meta = (BlockMeta*)malloc(numBlocks * numBlocks * sizeof(BlockMeta));
    BlockMeta* B_meta = (BlockMeta*)malloc(numBlocks * numBlocks * sizeof(BlockMeta));
    
    if (!A_meta || !B_meta) {
        free(A_meta);
        free(B_meta);
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    n, n, n, 1.0, A, n, B, n, 0.0, C, n);
        return AlmaError::Success;
    }

    #pragma omp parallel for collapse(2) schedule(dynamic, 1)
    for (int bi = 0; bi < numBlocks; ++bi) {
        for (int bj = 0; bj < numBlocks; ++bj) {
            double* Cb = &C[(bi * blockSize) * n + bj * blockSize];
            bool first = true;

            for (int bk = 0; bk < numBlocks; ++bk) {
                const double* Ab = &A[(bi * blockSize) * n + bk * blockSize];
                const double* Bb = &B[(bk * blockSize) * n + bj * blockSize];

                double alpha = first ? 0.0 : 1.0;
                
                cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                            blockSize, blockSize, blockSize,
                            1.0, Ab, n, Bb, n, alpha, Cb, n);
                first = false;
            }
        }
    }

    free(A_meta);
    free(B_meta);

    return AlmaError::Success;
}

AlmaError alma_multiply(double* A, double* B, double* C,
                        int n, int blockSize) {
    if (!A || !B || !C) {
        return AlmaError::NullPointer;
    }
    return alma_multiply_full(A, B, C, n, blockSize);
}

static double benchmark_config(double* A, double* B, double* C, int n, int blockSize, int repeats) {
    double min_time = 1e9;
    
    for (int r = 0; r < repeats; ++r) {
        std::memset(C, 0, n * n * sizeof(double));
        
        auto start = std::chrono::high_resolution_clock::now();
        alma_multiply_full(A, B, C, n, blockSize);
        auto end = std::chrono::high_resolution_clock::now();
        
        double elapsed = std::chrono::duration<double>(end - start).count();
        if (elapsed < min_time) min_time = elapsed;
    }
    return min_time;
}

static double benchmark_openblas(double* A, double* B, double* C, int n, int repeats) {
    double min_time = 1e9;
    
    for (int r = 0; r < repeats; ++r) {
        std::memset(C, 0, n * n * sizeof(double));
        
        auto start = std::chrono::high_resolution_clock::now();
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    n, n, n, 1.0, A, n, B, n, 0.0, C, n);
        auto end = std::chrono::high_resolution_clock::now();
        
        double elapsed = std::chrono::duration<double>(end - start).count();
        if (elapsed < min_time) min_time = elapsed;
    }
    return min_time;
}

static void tune_configuration() {
    if (g_tuned_config.initialized) return;
    
    int test_sizes[] = {512, 1024};
    int test_blocks[] = {64, 128, 256};
    
    for (int test_n : test_sizes) {
        std::vector<double> A(test_n * test_n), B(test_n * test_n), C(test_n * test_n);
        for (int i = 0; i < test_n * test_n; ++i) {
            A[i] = (i % 7) * 0.1;
            B[i] = (i % 5) * 0.2;
        }
        
        double openblas_time = benchmark_openblas(A.data(), B.data(), C.data(), test_n, 2);
        
        double best_time = openblas_time;
        int best_block = 0;
        
        for (int block : test_blocks) {
            if (test_n % block != 0) continue;
            
            double alma_time = benchmark_config(A.data(), B.data(), C.data(), test_n, block, 2);
            
            if (alma_time < best_time) {
                best_time = alma_time;
                best_block = block;
            }
        }
        
        if (best_block > 0) {
            g_tuned_config.blockSize = best_block;
            g_tuned_config.useOpenBLAS = false;
            g_tuned_config.initialized = true;
            return;
        }
    }
    
    g_tuned_config.useOpenBLAS = true;
    g_tuned_config.initialized = true;
}

AlmaError alma_multiply_auto(double* A, double* B, double* C, int n) {
    if (!A || !B || !C) {
        return AlmaError::NullPointer;
    }
    
    if (n <= 0) {
        return AlmaError::InvalidDimension;
    }
    
    if (!g_tuned_config.initialized) {
        std::lock_guard<std::mutex> lock(g_tuning_mutex);
        if (!g_tuned_config.initialized) {
            tune_configuration();
        }
    }
    
    if (g_tuned_config.useOpenBLAS) {
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    n, n, n, 1.0, A, n, B, n, 0.0, C, n);
        return AlmaError::Success;
    }
    
    int blockSize = g_tuned_config.blockSize;
    if (n % blockSize != 0) {
        blockSize = 128;
        if (n % blockSize != 0) {
            blockSize = n;
        }
    }
    return alma_multiply_full(A, B, C, n, blockSize);
}

int alma_get_optimal_block_size() {
    return get_cache_optimal_block_size();
}

void alma_multiply_block(const MatrixBlock& A,
                         const MatrixBlock& B,
                         MatrixBlock& C) {
    if (A.meta.type == BlockType::LowRank && B.meta.type == BlockType::LowRank) {
        multiply_lowrank_block(A, B, C);
    } else {
        multiply_dense_block(A, B, C);
    }
}

void multiply_dense_block(const MatrixBlock& A,
                          const MatrixBlock& B,
                          MatrixBlock& C) {
    if (!A.data || !B.data || !C.data) {
        return;
    }

    int ldA = A.meta.cols;
    int ldB = B.meta.cols;
    int ldC = C.meta.cols;

    cblas_dgemm(CblasRowMajor,
                CblasNoTrans, CblasNoTrans,
                A.meta.rows, B.meta.cols, A.meta.cols,
                1.0,
                A.data, ldA,
                B.data, ldB,
                1.0,
                C.data, ldC);
}

static bool safe_multiply_size(int a, int b, size_t* out) {
    if (a <= 0 || b <= 0) {
        *out = 0;
        return false;
    }
    if (a > INT_MAX / b) {
        return false;
    }
    *out = static_cast<size_t>(a) * static_cast<size_t>(b);
    return true;
}

void multiply_lowrank_block(const MatrixBlock& A,
                           const MatrixBlock& B,
                           MatrixBlock& C) {
    if (!A.data || !B.data || !C.data) {
        return;
    }

    int m = A.meta.rows;
    int n = B.meta.cols;
    int k = A.meta.cols;
    int ra = A.meta.rank;
    int rb = B.meta.rank;
    
    if (ra == 0 || rb == 0) {
        for (int i = 0; i < m * n; ++i) {
            C.data[i] = 0.0;
        }
        return;
    }

    size_t m_size, temp_size;
    if (!safe_multiply_size(ra, rb, &m_size) || 
        !safe_multiply_size(ra, n, &temp_size)) {
        multiply_dense_block(A, B, C);
        return;
    }

    double* M = (double*)malloc(m_size * sizeof(double));
    double* temp = (double*)malloc(temp_size * sizeof(double));
    
    if (!M || !temp) {
        free(M);
        free(temp);
        multiply_dense_block(A, B, C);
        return;
    }

    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                ra, rb, k, 1.0, A.meta.VT, k, B.meta.U, k, 0.0, M, rb);

    for (int i = 0; i < ra; ++i) {
        for (int j = 0; j < rb; ++j) {
            M[i * rb + j] *= A.meta.S[i] * B.meta.S[j];
        }
    }

    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                ra, n, rb, 1.0, M, rb, B.meta.VT, n, 0.0, temp, n);

    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                m, n, ra, 1.0, A.meta.U, ra, temp, n, 1.0, C.data, C.meta.cols);

    free(M);
    free(temp);
}
