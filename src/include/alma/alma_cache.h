#pragma once

#include <cstddef>

size_t alma_get_l3_cache_size();

int alma_get_numa_node();
int alma_get_num_numa_nodes();
int alma_set_thread_affinity(int cpu_id);
int alma_set_thread_affinity_by_numa(int numa_node, int num_threads);
