#!/bin/bash

source "$(dirname "$0")/build-common.sh"

detect_blas() {
    local blas_paths=(
        "/usr/lib/x86_64-linux-gnu/libopenblas.so"
        "/usr/lib/libopenblas.so"
        "/usr/lib64/libopenblas.so"
        "/usr/lib/x86_64-linux-gnu/openblas"
        "/usr/lib/openblas"
    )
    
    for path in "${blas_paths[@]}"; do
        if [ -e "$path" ]; then
            echo "OpenBLAS found at: $path"
            return 0
        fi
    done
    
    if pkg-config --exists openblas 2>/dev/null; then
        echo "OpenBLAS found via pkg-config"
        return 0
    fi
    
    if pkg-config --exists mkl 2>/dev/null; then
        echo "Intel MKL found via pkg-config"
        return 0
    fi
    
    return 1
}

get_linux_deps() {
    echo "Linux dependencies:"
    echo "  - meson (build system)"
    echo "  - ninja-build"
    echo "  - libopenblas-dev (default BLAS)"
    echo "  - libomp-dev (OpenMP)"
    echo "  - liblapacke-dev (LAPACK)"
    echo ""
    echo "Install with: sudo apt-get install meson ninja-build libopenblas-dev libomp-dev liblapacke-dev"
    echo ""
    echo "Optional - for best performance, install Intel MKL:"
    echo "  wget https://apt.repos.intel.com/oneapi/intel-oneapi-repo-1.0-1_all.deb"
    echo "  sudo dpkg -i intel-oneapi-repo-1.0-1_all.deb"
    echo "  sudo apt-get install -y intel-oneapi-mkl"
}
