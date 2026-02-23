#include "alma/alma_cache.h"
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <dirent.h>
#include <cstring>
#include <sched.h>
#include <unistd.h>
#include <vector>

static size_t parse_cache_size(const std::string& line) {
    if (line.empty()) return 0;
    size_t value = std::stoull(line.substr(0, line.size() - 1));
    char unit = line.back();
    if (unit == 'K' || unit == 'k') return value * 1024;
    if (unit == 'M' || unit == 'm') return value * 1024 * 1024;
    if (unit == 'G' || unit == 'g') return value * 1024 * 1024 * 1024;
    return value;
}

static bool is_numeric(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

size_t alma_get_l3_cache_size() {
    const char* cpu_path = "/sys/devices/system/cpu";
    size_t max_cache = 0;
    DIR* dir = opendir(cpu_path);
    if (!dir) return 8UL * 1024 * 1024;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.substr(0, 3) == "cpu" && is_numeric(name.substr(3))) {
            std::string cache_path = std::string(cpu_path) + "/" + name + "/cache/index3/size";
            std::ifstream cache_file(cache_path);
            if (cache_file) {
                std::string line;
                if (std::getline(cache_file, line)) {
                    size_t cache = parse_cache_size(line);
                    if (cache > max_cache) max_cache = cache;
                }
            }
        }
    }
    closedir(dir);

    if (max_cache == 0) {
        std::ifstream caches("/sys/devices/system/cpu/cpu0/cache/index3/size");
        std::string line;
        if (std::getline(caches, line)) {
            max_cache = parse_cache_size(line);
        }
    }

    return max_cache > 0 ? max_cache : 8UL * 1024 * 1024;
}

int alma_get_numa_node() {
    int cpu = sched_getcpu();
    if (cpu < 0) return 0;
    
    std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/topology/physical_package_id";
    std::ifstream file(path);
    if (file) {
        int node;
        if (file >> node) return node;
    }
    return 0;
}

int alma_get_num_numa_nodes() {
    const char* node_path = "/sys/devices/system/node";
    DIR* dir = opendir(node_path);
    if (!dir) return 1;
    
    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.substr(0, 4) == "node" && is_numeric(name.substr(4))) {
            count++;
        }
    }
    closedir(dir);
    return count > 0 ? count : 1;
}

int alma_set_thread_affinity(int cpu_id) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);
    return sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}

int alma_set_thread_affinity_by_numa(int numa_node, int num_threads) {
    const char* cpu_path = "/sys/devices/system/cpu";
    std::vector<int> numa_cpus;
    
    DIR* dir = opendir(cpu_path);
    if (!dir) return -1;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.substr(0, 3) == "cpu" && is_numeric(name.substr(3))) {
            int cpu_id = std::stoi(name.substr(3));
            std::string pkg_path = std::string(cpu_path) + "/" + name + "/topology/physical_package_id";
            std::ifstream pkg_file(pkg_path);
            if (pkg_file) {
                int pkg;
                if (pkg_file >> pkg && pkg == numa_node) {
                    numa_cpus.push_back(cpu_id);
                }
            }
        }
    }
    closedir(dir);
    
    if (numa_cpus.empty()) return -1;
    
    cpu_set_t mask;
    CPU_ZERO(&mask);
    int count = std::min(num_threads, (int)numa_cpus.size());
    for (int i = 0; i < count; ++i) {
        CPU_SET(numa_cpus[i], &mask);
    }
    
    return sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}
