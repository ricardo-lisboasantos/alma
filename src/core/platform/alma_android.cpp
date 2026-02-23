#include "alma/alma_cache.h"
#include <sys/sysconf.h>
#include <pthread.h>
#include <unistd.h>

size_t alma_get_l3_cache_size() {
    long cache = sysconf(_SC_LEVEL3_CACHE_SIZE);
    if (cache > 0) return static_cast<size_t>(cache);
    
    cache = sysconf(_SC_LEVEL2_CACHE_SIZE);
    if (cache > 0) return static_cast<size_t>(cache);
    
    return 2UL * 1024 * 1024;
}

int alma_get_numa_node() {
    return 0;
}

int alma_get_num_numa_nodes() {
    return 1;
}

int alma_set_thread_affinity(int cpu_id) {
#if defined(__NR_sched_setaffinity)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    return sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
#else
    return 0;
#endif
}

int alma_set_thread_affinity_by_numa(int numa_node, int num_threads) {
    return 0;
}
