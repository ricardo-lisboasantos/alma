#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <cblas.h>
#include <lapacke.h>
#include <random>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <iomanip>
#include <functional>
#include <omp.h>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif
#include <sys/types.h>
#include <thread>

using highres_clock = std::chrono::high_resolution_clock;
using duration_ms = std::chrono::duration<double, std::milli>;

double gflops(int n, double time_ms) {
    if (time_ms <= 0) return 0.0;
    return 2.0 * n * n * n / (time_ms * 1e6);
}

double gflops_factorize(int n, double time_ms) {
    if (time_ms <= 0) return 0.0;
    return (4.0/3.0) * n * n * n / (time_ms * 1e6);
}

struct SystemInfo {
    int physical_cores;
    int logical_cores;
    size_t total_ram_bytes;
    size_t l3_cache_bytes;
    std::string backend;
};

static SystemInfo detect_system_info() {
    SystemInfo info = {1, 1, 0, 0, "unknown"};
    
    const char* backend = getenv("ALMA_BACKEND");
    if (backend) {
        info.backend = backend;
    } else {
        info.backend = "openblas";
    }

#if defined(__APPLE__)
    size_t len = sizeof(info.physical_cores);
    sysctlbyname("hw.physicalcpu", &info.physical_cores, &len, NULL, 0);
    sysctlbyname("hw.logicalcpu", &info.logical_cores, &len, NULL, 0);

    size_t ram_size = 0;
    len = sizeof(ram_size);
    sysctlbyname("hw.memsize", &ram_size, &len, NULL, 0);
    info.total_ram_bytes = ram_size;

    int64_t l3size = 0;
    len = sizeof(l3size);
    sysctlbyname("hw.l3cachesize", &l3size, &len, NULL, 0);
    info.l3_cache_bytes = (size_t)l3size;
    info.backend = "accelerate";
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
    if (cores > 0) info.physical_cores = cores;
    info.logical_cores = std::thread::hardware_concurrency();

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

    info.physical_cores = info.physical_cores > 0 ? info.physical_cores : 1;
    info.logical_cores = info.logical_cores > 0 ? info.logical_cores : 1;
    if (info.total_ram_bytes == 0) info.total_ram_bytes = 8UL * 1024 * 1024 * 1024;
    if (info.l3_cache_bytes == 0) info.l3_cache_bytes = 8UL * 1024 * 1024;

    return info;
}

static void print_system_info(const SystemInfo& info) {
    std::cerr << "=== System Configuration ===" << std::endl;
    std::cerr << "Backend:       " << info.backend << std::endl;
    std::cerr << "Physical cores: " << info.physical_cores << std::endl;
    std::cerr << "Logical cores:  " << info.logical_cores << std::endl;
    std::cerr << "Total RAM:      " << (info.total_ram_bytes / (1024UL * 1024 * 1024)) << " GB" << std::endl;
    std::cerr << "L3 cache:       " << (info.l3_cache_bytes / 1024) << " KB" << std::endl;
    std::cerr << "===========================" << std::endl;
}

double time_kernel(std::function<void()> f, int repeats, bool warmup = true) {
    if (warmup) {
        for (int i = 0; i < 2; ++i) f();
    }

    auto t0 = highres_clock::now();
    for (int i = 0; i < repeats; ++i) f();
    auto t1 = highres_clock::now();

    return std::chrono::duration_cast<duration_ms>(t1 - t0).count() / repeats;
}

void random_fill(std::vector<double>& M, std::mt19937& gen) {
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    for (size_t i = 0; i < M.size(); ++i) {
        M[i] = dist(gen);
    }
}

void print_usage(const char* name) {
    std::cout << "Usage: " << name << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -s N          Matrix size (default: 1024)\n";
    std::cout << "  -r N          Repeats (default: 3)\n";
    std::cout << "  -t N          Number of threads (default: physical cores)\n";
    std::cout << "  -o, --op      Operation: mul, lu, qr, svd, inv (default: mul)\n";
    std::cout << "  --csv         CSV output\n";
    std::cout << "  --sweep       Run size sweep for current operation\n";
    std::cout << "  -v            Verbose output\n";
    std::cout << "  --sysinfo     Print system info and exit\n";
}

void run_matmul_benchmark(int n, int repeats, bool csv, bool verbose) {
    std::mt19937 gen(42);
    std::vector<double> A(n * n), B(n * n), C(n * n);
    random_fill(A, gen);
    random_fill(B, gen);

    auto func = [&]() {
        std::fill(C.begin(), C.end(), 0.0);
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    n, n, n, 1.0, A.data(), n, B.data(), n, 0.0, C.data(), n);
    };

    double ms = time_kernel(func, repeats);

    if (csv) {
        std::cout << "mul," << n << ",," << std::fixed << std::setprecision(4) << ms 
                  << "," << gflops(n, ms) << "\n";
    } else {
        std::cout << "Matrix multiplication: " << n << "x" << n << "\n";
        std::cout << "  Time:  " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  FLOPS: " << std::fixed << std::setprecision(1) << gflops(n, ms) << " GFLOPS\n";
    }
}

void run_lu_benchmark(int n, int repeats, bool csv, bool verbose) {
    std::mt19937 gen(42);
    std::vector<double> A(n * n);
    random_fill(A, gen);

    std::vector<int> ipiv(n);
    int lda = n;

    auto func = [&]() {
        std::vector<double> copy = A;
        LAPACKE_dgetrf(LAPACK_ROW_MAJOR, n, n, copy.data(), lda, ipiv.data());
    };

    double ms = time_kernel(func, repeats);

    if (csv) {
        std::cout << "lu," << n << ",," << std::fixed << std::setprecision(4) << ms 
                  << "," << gflops_factorize(n, ms) << "\n";
    } else {
        std::cout << "LU decomposition: " << n << "x" << n << "\n";
        std::cout << "  Time:  " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  FLOPS: " << std::fixed << std::setprecision(1) << gflops_factorize(n, ms) << " GFLOPS\n";
    }
}

void run_qr_benchmark(int n, int repeats, bool csv, bool verbose) {
    std::mt19937 gen(42);
    std::vector<double> A(n * n);
    random_fill(A, gen);

    int lda = n;
    int min_dim = std::min(n, n);
    std::vector<double> tau(min_dim);
    int lwork = n * 64;
    std::vector<double> work(lwork);

    auto func = [&]() {
        std::vector<double> copy = A;
        LAPACKE_dgeqrf_work(LAPACK_ROW_MAJOR, n, n, copy.data(), lda, tau.data(), work.data(), lwork);
    };

    double ms = time_kernel(func, repeats);

    if (csv) {
        std::cout << "qr," << n << ",," << std::fixed << std::setprecision(4) << ms 
                  << "," << gflops_factorize(n, ms) << "\n";
    } else {
        std::cout << "QR decomposition: " << n << "x" << n << "\n";
        std::cout << "  Time:  " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  FLOPS: " << std::fixed << std::setprecision(1) << gflops_factorize(n, ms) << " GFLOPS\n";
    }
}

void run_svd_benchmark(int n, int repeats, bool csv, bool verbose) {
    std::mt19937 gen(42);
    std::vector<double> A(n * n);
    random_fill(A, gen);

    int lda = n;
    int min_dim = std::min(n, n);
    std::vector<double> S(min_dim);
    std::vector<double> U(n * min_dim);
    std::vector<double> VT(min_dim * n);
    std::vector<double> superb(min_dim - 1);

    auto func = [&]() {
        std::vector<double> copy = A;
        LAPACKE_dgesvd(LAPACK_ROW_MAJOR, 'S', 'S', n, n, copy.data(), lda,
                       S.data(), U.data(), n, VT.data(), n, superb.data());
    };

    double ms = time_kernel(func, repeats);

    if (csv) {
        std::cout << "svd," << n << ",," << std::fixed << std::setprecision(4) << ms 
                  << "," << gflops_factorize(n, ms) << "\n";
    } else {
        std::cout << "SVD: " << n << "x" << n << "\n";
        std::cout << "  Time:  " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  FLOPS: " << std::fixed << std::setprecision(1) << gflops_factorize(n, ms) << " GFLOPS\n";
    }
}

void run_inv_benchmark(int n, int repeats, bool csv, bool verbose) {
    std::mt19937 gen(42);
    std::vector<double> A(n * n);
    random_fill(A, gen);
    
    for (int i = 0; i < n; ++i) {
        A[i * n + i] += n;
    }

    std::vector<int> ipiv(n);
    int lda = n;
    int lwork = n * 64;
    std::vector<double> work(lwork);

    auto func = [&]() {
        std::vector<double> invA = A;
        LAPACKE_dgetrf(LAPACK_ROW_MAJOR, n, n, invA.data(), lda, ipiv.data());
        LAPACKE_dgetri_work(LAPACK_ROW_MAJOR, n, invA.data(), lda, ipiv.data(), work.data(), lwork);
    };

    double ms = time_kernel(func, repeats);

    if (csv) {
        std::cout << "inv," << n << ",," << std::fixed << std::setprecision(4) << ms 
                  << "," << gflops_factorize(n, ms) << "\n";
    } else {
        std::cout << "Matrix inverse: " << n << "x" << n << "\n";
        std::cout << "  Time:  " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  FLOPS: " << std::fixed << std::setprecision(1) << gflops_factorize(n, ms) << " GFLOPS\n";
    }
}

void run_sweep(const std::string& op, bool csv) {
    if (csv) {
        std::cout << "operation,n,time_ms,gflops\n";
    } else {
        std::cout << "=== Size Sweep: " << op << " ===\n\n";
    }

    std::vector<int> sizes = {128, 256, 512, 1024, 2048, 4096};

    for (int n : sizes) {
        if (op == "mul" || op == "all") run_matmul_benchmark(n, 3, csv, false);
        if (op == "lu" || op == "all") run_lu_benchmark(n, 3, csv, false);
        if (op == "qr" || op == "all") run_qr_benchmark(n, 3, csv, false);
        if (op == "svd" || op == "all") run_svd_benchmark(n, 3, csv, false);
        if (op == "inv" || op == "all") run_inv_benchmark(n, 3, csv, false);
    }
}

int main(int argc, char** argv) {
    int n = 1024;
    int repeats = 3;
    int threads = 0;
    bool csv = false;
    bool verbose = false;
    bool sweep = false;
    std::string op = "mul";

    SystemInfo sys = detect_system_info();

    if (argc >= 2 && std::string(argv[1]) == "--sysinfo") {
        print_system_info(sys);
        return 0;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-s" && i + 1 < argc) {
            n = std::atoi(argv[++i]);
        } else if (arg == "-r" && i + 1 < argc) {
            repeats = std::atoi(argv[++i]);
        } else if (arg == "-t" && i + 1 < argc) {
            threads = std::atoi(argv[++i]);
        } else if (arg == "-o" && i + 1 < argc) {
            op = argv[++i];
        } else if (arg == "--op" && i + 1 < argc) {
            op = argv[++i];
        } else if (arg == "--csv") {
            csv = true;
        } else if (arg == "--sweep") {
            sweep = true;
        } else if (arg == "--sysinfo") {
            print_system_info(sys);
            return 0;
        } else if (arg == "-v") {
            verbose = true;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (threads > 0) {
        omp_set_num_threads(threads);
    }

    if (!verbose && !csv) {
        print_system_info(sys);
    }

    if (sweep) {
        run_sweep(op, csv);
        return 0;
    }

    if (csv) {
        std::cout << "operation,n,time_ms,gflops\n";
    }

    if (op == "mul") {
        run_matmul_benchmark(n, repeats, csv, verbose);
    } else if (op == "lu") {
        run_lu_benchmark(n, repeats, csv, verbose);
    } else if (op == "qr") {
        run_qr_benchmark(n, repeats, csv, verbose);
    } else if (op == "svd") {
        run_svd_benchmark(n, repeats, csv, verbose);
    } else if (op == "inv") {
        run_inv_benchmark(n, repeats, csv, verbose);
    } else {
        std::cerr << "Unknown operation: " << op << "\n";
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
