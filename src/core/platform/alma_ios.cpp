#include "alma/alma_cache.h"
#include <sys/sysctl.h>
#include <pthread.h>

size_t alma_get_l3_cache_size() {
    int64_t l3size = 0;
    size_t len = sizeof(l3size);
    sysctlbyname("hw.l3cachesize", &l3size, &len, NULL, 0);
    return static_cast<size_t>(l3size);
}

int alma_get_numa_node() {
    return 0;
}

int alma_get_num_numa_nodes() {
    return 1;
}

int alma_set_thread_affinity(int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

int alma_set_thread_affinity_by_numa(int numa_node, int num_threads) {
    return 0;
}
