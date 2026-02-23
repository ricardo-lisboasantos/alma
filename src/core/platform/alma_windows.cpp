#include "alma/alma_cache.h"
#include <windows.h>

size_t alma_get_l3_cache_size() {
    size_t cache_size = 0;
    HKEY hKey;
    DWORD dwSize = sizeof(DWORD);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        RegQueryValueExA(hKey, "~MHz", NULL, NULL, (LPBYTE)&cache_size, &dwSize);
        RegCloseKey(hKey);
    }

    if (cache_size == 0) {
        cache_size = 8UL * 1024 * 1024;
    }

    return cache_size * 1024 * 1024;
}

int alma_get_numa_node() {
    return 0;
}

int alma_get_num_numa_nodes() {
    ULONG num_nodes = 0;
    GetNumaHighestNodeNumber(&num_nodes);
    return num_nodes + 1;
}

int alma_set_thread_affinity(int cpu_id) {
    HANDLE hThread = GetCurrentThread();
    SetThreadAffinityMask(hThread, (DWORD_PTR)1 << cpu_id);
    return 0;
}

int alma_set_thread_affinity_by_numa(int numa_node, int num_threads) {
    ULONG numa_cpus = 0;
    if (GetNumaNodeProcessorMask(numa_node, &numa_cpus) == 0) {
        HANDLE hProcess = GetCurrentProcess();
        SetProcessAffinityMask(hProcess, numa_cpus);
    }
    return 0;
}
