#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <cblas.h>
#include <random>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <functional>
#include <fstream>
#include <sstream>
#include "../src/alma.h"
#include <omp.h>
#include <thread>
#include <string>
#include <cstdlib>
#include <sys/sysctl.h>
#include <sys/types.h>

struct SystemInfo {
    int physical_cores;
    int logical_cores;
    size_t total_ram_bytes;
    size_t l3_cache_bytes;
};

static SystemInfo detect_system_info() {
    SystemInfo info = {1, 1, 0, 0};
    
    int physical_cores = 1, logical_cores = 1;
    
#if defined(__APPLE__)
    size_t len = sizeof(physical_cores);
    sysctlbyname("hw.physicalcpu", &physical_cores, &len, NULL, 0);
    sysctlbyname("hw.logicalcpu", &logical_cores, &len, NULL, 0);
    
    size_t ram_size = 0;
    len = sizeof(ram_size);
    sysctlbyname("hw.memsize", &ram_size, &len, NULL, 0);
    info.total_ram_bytes = ram_size;
    
    int64_t l3size = 0;
    len = sizeof(l3size);
    sysctlbyname("hw.l3cachesize", &l3size, &len, NULL, 0);
    info.l3_cache_bytes = (size_t)l3size;
#elif defined(__linux__)
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    int physical = 0, cores = 0;
    while (std::getline(cpuinfo, line)) {
        if (line.find("physical id") != std::string::npos) {
            physical = std::stoi(line.substr(line.find(':') + 1));
        }
        if (line.find("cpu cores") != std::string::npos) {
            cores = std::stoi(line.substr(line.find(':') + 1));
        }
    }
    if (cores > 0) physical_cores = cores;
    logical_cores = std::thread::hardware_concurrency();
    
    std::ifstream meminfo("/proc/meminfo");
    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") != std::string::npos) {
            info.total_ram_bytes = std::stoull(line.substr(line.find(':') + 1)) * 1024;
            break;
        }
    }
    
    std::ifstream caches("/sys/devices/system/cpu/cpu0/cache/index3/size");
    if (std::getline(caches, line)) {
        size_t cache = std::stoull(line.substr(0, line.size() - 1));
        info.l3_cache_bytes = (line.back() == 'K') ? cache * 1024 : cache * 1024 * 1024;
    }
#endif
    
    info.physical_cores = physical_cores > 0 ? physical_cores : 1;
    info.logical_cores = logical_cores > 0 ? logical_cores : 1;
    if (info.total_ram_bytes == 0) info.total_ram_bytes = 8UL * 1024 * 1024 * 1024;
    if (info.l3_cache_bytes == 0) info.l3_cache_bytes = 8UL * 1024 * 1024;
    
    return info;
}

static int get_default_block_size(const SystemInfo& info) {
    size_t cache = info.l3_cache_bytes;
    int block = 32;
    while ((size_t)block * block * sizeof(double) * 2 < cache / 4 && block < 512) {
        block *= 2;
    }
    return block;
}

static void print_system_info(const SystemInfo& info) {
    std::cerr << "=== System Configuration ===" << std::endl;
    std::cerr << "Physical cores: " << info.physical_cores << std::endl;
    std::cerr << "Logical cores:  " << info.logical_cores << std::endl;
    std::cerr << "Total RAM:      " << (info.total_ram_bytes / (1024UL * 1024 * 1024)) << " GB" << std::endl;
    std::cerr << "L3 cache:       " << (info.l3_cache_bytes / 1024) << " KB" << std::endl;
    std::cerr << "Recomended block size: " << get_default_block_size(info) << std::endl;
    std::cerr << "===========================" << std::endl;
}

bool load_csv(const std::string& path, std::vector<double>& M, int& n) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << path << "\n";
        return false;
    }
    
    std::vector<std::vector<double>> rows;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::vector<double> row;
        double val;
        while (ss >> val) {
            row.push_back(val);
            if (ss.peek() == ',') ss.ignore();
        }
        if (!row.empty()) rows.push_back(std::move(row));
    }
    
    if (rows.empty()) {
        std::cerr << "Error: Empty CSV file " << path << "\n";
        return false;
    }
    
    n = rows.size();
    M.resize(n * n);
    for (int i = 0; i < n; ++i) {
        if ((int)rows[i].size() != n) {
            std::cerr << "Error: Matrix " << path << " is not square or has inconsistent rows\n";
            return false;
        }
        for (int j = 0; j < n; ++j) {
            M[i * n + j] = rows[i][j];
        }
    }
    return true;
}

using highres_clock = std::chrono::high_resolution_clock;
using duration_ms = std::chrono::duration<double, std::milli>;

double time_kernel(std::function<void()> f, int repeats, bool warmup = true) {
    if (warmup) {
        for (int i = 0; i < 2; ++i) f();
    }

    auto t0 = highres_clock::now();
    for (int i = 0; i < repeats; ++i) f();
    auto t1 = highres_clock::now();
    
    return std::chrono::duration_cast<duration_ms>(t1 - t0).count() / repeats;
}

// Helpers to avoid thread-pool interference between OpenBLAS and alma (OpenMP).
// We configure environment variables and OpenMP thread count so each kernel
// uses the intended threading model and they don't compete for CPU resources.

static void set_env_if_present(const char* name, const std::string& value) {
    // setenv returns 0 on success; ignore errors — best-effort only
    setenv(name, value.c_str(), 1);
}

static std::string get_cpu_affinity_string(int threads) {
    std::ostringstream oss;
    for (int i = 0; i < threads; ++i) {
        if (i > 0) oss << ",";
        oss << i;
    }
    return oss.str();
}

static void configure_for_openblas(int threads, bool use_affinity) {
    set_env_if_present("OPENBLAS_NUM_THREADS", std::to_string(threads));
    set_env_if_present("MKL_NUM_THREADS", std::to_string(threads));
    set_env_if_present("VECLIB_MAXIMUM_THREADS", std::to_string(threads));
    set_env_if_present("OMP_NUM_THREADS", "1");
    omp_set_num_threads(1);
    
    if (use_affinity) {
        set_env_if_present("GOMP_CPU_AFFINITY", get_cpu_affinity_string(threads));
        set_env_if_present("OMP_PROC_BIND", "close");
    }
}

static void configure_for_alma(int threads, bool use_affinity) {
    set_env_if_present("OPENBLAS_NUM_THREADS", "1");
    set_env_if_present("MKL_NUM_THREADS", "1");
    set_env_if_present("VECLIB_MAXIMUM_THREADS", "1");
    set_env_if_present("OMP_NUM_THREADS", std::to_string(threads));
    omp_set_num_threads(threads);
    
    if (use_affinity) {
        set_env_if_present("GOMP_CPU_AFFINITY", get_cpu_affinity_string(threads));
        set_env_if_present("OMP_PROC_BIND", "close");
    }
}

static void print_thread_config(const std::string& tag, int threads, bool use_affinity) {
    const char* ob = std::getenv("OPENBLAS_NUM_THREADS");
    const char* mkl = std::getenv("MKL_NUM_THREADS");
    const char* veclib = std::getenv("VECLIB_MAXIMUM_THREADS");
    const char* ompenv = std::getenv("OMP_NUM_THREADS");
    const char* affinity = std::getenv("GOMP_CPU_AFFINITY");
    int omp_threads = omp_get_max_threads();
    std::cerr << "[" << tag << "] threads=" << threads
              << " omp_max_threads=" << omp_threads
              << " affinity=" << (use_affinity ? (affinity ? affinity : "enabled") : "disabled")
              << " OPENBLAS_NUM_THREADS=" << (ob ? ob : "(unset)")
              << " MKL_NUM_THREADS=" << (mkl ? mkl : "(unset)")
              << " OMP_NUM_THREADS=" << (ompenv ? ompenv : "(unset)")
              << std::endl;
}

void random_fill(std::vector<double>& M, std::mt19937& gen) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (size_t i = 0; i < M.size(); ++i) {
        M[i] = dist(gen);
    }
}

void openblas_mul(const double* A, const double* B, double* C, int n) {
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                n, n, n,
                1.0, A, n, B, n, 0.0, C, n);
}

double gflops(int n, double time_ms) {
    return 2.0 * n * n * n / (time_ms * 1e6);
}

void print_usage(const char* name) {
    std::cout << "Usage: " << name << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -s N          Matrix size (default: 1024)\n";
    std::cout << "  -b N          Block size (default: 128)\n";
    std::cout << "  -r N          Repeats (default: 3)\n";
    std::cout << "  -a FILE       Load matrix A from CSV (default: random)\n";
    std::cout << "  -B FILE       Load matrix B from CSV (default: random)\n";
    std::cout << "  --sweep       Run sweep over sizes and blocks\n";
    std::cout << "  --csv-bench   Run benchmarks with all CSV matrices\n";
    std::cout << "  -v            Verbose output\n";
    std::cout << "  -t N          Number of threads (default: physical cores)\n";
    std::cout << "  --no-affinity Disable CPU affinity binding\n";
    std::cout << "  --sysinfo     Print system info and exit\n";
}

int main(int argc, char** argv) {
    int n = 1024;
    int block = 128;
    int repeats = 3;
    int threads = 0;
    bool use_affinity = true;
    bool sweep = false;
    bool csv_bench = false;
    bool csv = false;
    bool verbose = false;
    bool sysinfo_only = false;
    std::string matrix_a_path, matrix_b_path;
    
    SystemInfo sys = detect_system_info();
    int default_threads = sys.physical_cores;
    block = get_default_block_size(sys);
    
    if (argc == 2 && std::string(argv[1]) == "--sysinfo") {
        print_system_info(sys);
        return 0;
    }
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-s" && i + 1 < argc) {
            n = std::atoi(argv[++i]);
        } else if (arg == "-b" && i + 1 < argc) {
            block = std::atoi(argv[++i]);
        } else if (arg == "-r" && i + 1 < argc) {
            repeats = std::atoi(argv[++i]);
        } else if (arg == "-t" && i + 1 < argc) {
            threads = std::atoi(argv[++i]);
        } else if (arg == "--no-affinity") {
            use_affinity = false;
        } else if (arg == "--sysinfo") {
            print_system_info(sys);
            return 0;
        } else if (arg == "-a" && i + 1 < argc) {
            matrix_a_path = argv[++i];
        } else if (arg == "-B" && i + 1 < argc) {
            matrix_b_path = argv[++i];
        } else if (arg == "--sweep") {
            sweep = true;
        } else if (arg == "--csv-bench") {
            csv_bench = true;
        } else if (arg == "--csv") {
            csv = true;
        } else if (arg == "-v") {
            verbose = true;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    if (threads <= 0) threads = default_threads;
    if (threads > sys.logical_cores) threads = sys.logical_cores;
    
    print_system_info(sys);
    
    std::mt19937 gen(42);
    
    std::vector<double> A, B;
    if (!matrix_a_path.empty() || !matrix_b_path.empty()) {
        int n_a = 0, n_b = 0;
        if (!matrix_a_path.empty()) {
            if (!load_csv(matrix_a_path, A, n_a)) return 1;
            n = n_a;
        }
        if (!matrix_b_path.empty()) {
            if (!load_csv(matrix_b_path, B, n_b)) return 1;
            if (!matrix_a_path.empty() && n_a != n_b) {
                std::cerr << "Error: Matrix dimensions mismatch\n";
                return 1;
            }
            n = n_b;
        }
        if (verbose) {
            std::cout << "Loaded matrices from CSV: " << n << "x" << n << "\n";
        }
    }
    
    if (sweep) {
        // CSV header
        if (csv) {
            std::cout << "n,block,openblas_ms,alma_ms,speedup,gflops_openblas,gflops_alma,maxdiff\n";
        } else {
            std::cout << "========================================\n";
            std::cout << "       BENCHMARK SWEEP\n";
            std::cout << "========================================\n\n";
        }
        
        std::vector<int> sizes = {256, 512, 1024, 2048};
        std::vector<int> blocks = {32, 64, 128, 256};
        
        for (int size : sizes) {
            for (int blk : blocks) {
                if (blk > size) continue;
                
                std::vector<double> A(size*size), B(size*size), C_alma(size*size), C_openblas(size*size);
                random_fill(A, gen);
                random_fill(B, gen);
                
                // Time OpenBLAS (configure threads so BLAS can be multithreaded and OpenMP is single-threaded)
                configure_for_openblas(threads, use_affinity);
                print_thread_config("OpenBLAS", threads, use_affinity);
                auto openblas_func = [&]() {
                    std::fill(C_openblas.begin(), C_openblas.end(), 0.0);
                    openblas_mul(A.data(), B.data(), C_openblas.data(), size);
                };
                double t_openblas = time_kernel(openblas_func, repeats);
                
                // Time alma (configure so alma/OpenMP uses multiple threads but BLAS calls inside are single-threaded)
                configure_for_alma(threads, use_affinity);
                print_thread_config("alma", threads, use_affinity);
                auto alma_func = [&]() {
                    std::fill(C_alma.begin(), C_alma.end(), 0.0);
                    alma_multiply(A.data(), B.data(), C_alma.data(), size, blk);
                };
                double t_alma = time_kernel(alma_func, repeats);
                
                // Verify
                double maxdiff = 0.0;
                for (int i = 0; i < size*size; ++i) {
                    maxdiff = std::max(maxdiff, std::fabs(C_openblas[i] - C_alma[i]));
                }
                
                double speedup = t_openblas / t_alma;
                
                if (csv) {
                    std::cout << size << ","
                              << blk << ","
                              << std::fixed << std::setprecision(4) << t_openblas << ","
                              << t_alma << ","
                              << speedup << ","
                              << gflops(size, t_openblas) << ","
                              << gflops(size, t_alma) << ","
                              << std::scientific << maxdiff << "\n";
                } else {
                    std::cout << "Size: " << size << "x" << size << ", Block: " << blk << "\n";
                    std::cout << "  OpenBLAS: " << std::fixed << std::setprecision(2) 
                              << t_openblas << " ms (" << gflops(size, t_openblas) << " GFLOPS)\n";
                    std::cout << "  alma:     " << t_alma << " ms (" << gflops(size, t_alma) << " GFLOPS)\n";
                    std::cout << "  Speedup:  " << speedup << "x\n";
                    std::cout << "  Max diff: " << std::scientific << maxdiff << "\n\n";
                }
            }
        }
    } else if (csv_bench) {
        if (csv) {
            std::cout << "matrix_a,matrix_b,n,block,openblas_ms,alma_ms,speedup,gflops_openblas,gflops_alma,maxdiff\n";
        } else {
            std::cout << "========================================\n";
            std::cout << "       CSV MATRIX BENCHMARK\n";
            std::cout << "========================================\n\n";
        }
        
        struct MatrixPair {
            std::string a, b;
            int n, block;
        };
        
        std::vector<MatrixPair> tests = {
            {"bench/data/matrix1.csv", "bench/data/matrix2.csv", 512, 128},
            {"bench/data/matrix_1024_random.csv", "bench/data/matrix_1024_random.csv", 1024, 128},
            {"bench/data/matrix_1024_sparse.csv", "bench/data/matrix_1024_sparse.csv", 1024, 128},
            {"bench/data/matrix_1024_identity.csv", "bench/data/matrix_1024_identity.csv", 1024, 128},
            {"bench/data/matrix_1024_banded.csv", "bench/data/matrix_1024_banded.csv", 1024, 128},
            {"bench/data/matrix_2048_random.csv", "bench/data/matrix_2048_random.csv", 2048, 256},
            {"bench/data/matrix_2048_sparse.csv", "bench/data/matrix_2048_sparse.csv", 2048, 256},
            {"bench/data/matrix_2048_identity.csv", "bench/data/matrix_2048_identity.csv", 2048, 256},
            {"bench/data/matrix_2048_banded.csv", "bench/data/matrix_2048_banded.csv", 2048, 256},
        };
        
        for (const auto& test : tests) {
            int n_a = 0, n_b = 0;
            std::vector<double> A, B;
            
            if (!load_csv(test.a, A, n_a)) {
                std::cerr << "Failed to load " << test.a << "\n";
                continue;
            }
            if (!load_csv(test.b, B, n_b)) {
                std::cerr << "Failed to load " << test.b << "\n";
                continue;
            }
            
            if (!csv) {
                std::cout << "Matrix A: " << test.a << " (" << n_a << "x" << n_a << ")\n";
                std::cout << "Matrix B: " << test.b << " (" << n_b << "x" << n_b << ")\n";
                std::cout << "Block size: " << test.block << "\n";
            }
            
            std::vector<double> C_alma(n_a * n_a), C_openblas(n_a * n_a);
            
            // Ensure OpenBLAS runs with BLAS threads enabled and OpenMP disabled
            configure_for_openblas(threads, use_affinity);
            print_thread_config("OpenBLAS", threads, use_affinity);
            auto openblas_func = [&]() {
                std::fill(C_openblas.begin(), C_openblas.end(), 0.0);
                openblas_mul(A.data(), B.data(), C_openblas.data(), n_a);
            };
            double t_openblas = time_kernel(openblas_func, repeats);
            
            // Ensure alma uses OpenMP threads and BLAS is single-threaded
            configure_for_alma(threads, use_affinity);
            print_thread_config("alma", threads, use_affinity);
            auto alma_func = [&]() {
                std::fill(C_alma.begin(), C_alma.end(), 0.0);
                alma_multiply(A.data(), B.data(), C_alma.data(), n_a, test.block);
            };
            double t_alma = time_kernel(alma_func, repeats);
            
            double maxdiff = 0.0;
            for (int i = 0; i < n_a * n_a; ++i) {
                maxdiff = std::max(maxdiff, std::fabs(C_openblas[i] - C_alma[i]));
            }
            
            double speedup = t_openblas / t_alma;
            
            if (csv) {
                std::cout << test.a << ","
                          << test.b << ","
                          << n_a << ","
                          << test.block << ","
                          << std::fixed << std::setprecision(4) << t_openblas << ","
                          << t_alma << ","
                          << speedup << ","
                          << gflops(n_a, t_openblas) << ","
                          << gflops(n_a, t_alma) << ","
                          << std::scientific << maxdiff << "\n";
            } else {
                std::cout << "  OpenBLAS: " << std::fixed << std::setprecision(2) 
                          << t_openblas << " ms (" << gflops(n_a, t_openblas) << " GFLOPS)\n";
                std::cout << "  alma:     " << t_alma << " ms (" << gflops(n_a, t_alma) << " GFLOPS)\n";
                std::cout << "  Speedup:  " << speedup << "x\n";
                std::cout << "  Max diff: " << std::scientific << maxdiff << "\n\n";
            }
        }
    } else {
        // Single benchmark
        std::vector<double> C_alma(n*n), C_openblas(n*n);
        
        if (A.empty()) {
            A.resize(n * n);
            random_fill(A, gen);
        }
        if (B.empty()) {
            B.resize(n * n);
            random_fill(B, gen);
        }
        
        if (verbose) {
            std::cout << "Matrix: " << n << "x" << n << ", Block: " << block << "\n";
            std::cout << "========================================\n";
        }
        
        // Time OpenBLAS: configure thread pools to avoid interference
        configure_for_openblas(threads, use_affinity);
        print_thread_config("OpenBLAS", threads, use_affinity);
        auto openblas_func = [&]() {
            std::fill(C_openblas.begin(), C_openblas.end(), 0.0);
            openblas_mul(A.data(), B.data(), C_openblas.data(), n);
        };
        double t_openblas = time_kernel(openblas_func, repeats);
        
        // Time alma: configure thread pools so alma/OpenMP runs multi-threaded
        configure_for_alma(threads, use_affinity);
        print_thread_config("alma", threads, use_affinity);
        auto alma_func = [&]() {
            std::fill(C_alma.begin(), C_alma.end(), 0.0);
            alma_multiply(A.data(), B.data(), C_alma.data(), n, block);
        };
        double t_alma = time_kernel(alma_func, repeats);
        
        // Verify
        double maxdiff = 0.0;
        for (int i = 0; i < n*n; ++i) {
            maxdiff = std::max(maxdiff, std::fabs(C_openblas[i] - C_alma[i]));
        }
        
        double speedup = t_openblas / t_alma;
        
        if (csv) {
            std::cout << "n,block,openblas_ms,alma_ms,speedup,gflops_openblas,gflops_alma,maxdiff\n";
            std::cout << n << ","
                      << block << ","
                      << std::fixed << std::setprecision(4) << t_openblas << ","
                      << t_alma << ","
                      << speedup << ","
                      << gflops(n, t_openblas) << ","
                      << gflops(n, t_alma) << ","
                      << std::scientific << maxdiff << "\n";
        } else if (!verbose) {
            std::cout << "Matrix: " << n << "x" << n << ", Block: " << block << "\n";
            std::cout << "========================================\n";
        }
        
        if (!csv || verbose) {
            std::cout << "OpenBLAS:  " << std::fixed << std::setprecision(2) 
                      << t_openblas << " ms (" << gflops(n, t_openblas) << " GFLOPS)\n";
            std::cout << "alma:      " << t_alma << " ms (" << gflops(n, t_alma) << " GFLOPS)\n";
            std::cout << "Speedup:   " << speedup << "x\n";
            std::cout << "Max diff:  " << std::scientific << maxdiff << "\n";
        }
    }
    
    return 0;
}
