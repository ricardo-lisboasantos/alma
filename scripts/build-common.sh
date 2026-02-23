#!/bin/bash

detect_os() {
    case "$(uname -s)" in
        Linux*)     echo "linux" ;;
        Darwin*)    echo "mac" ;;
        *)          echo "unknown" ;;
    esac
}

get_pkg_manager() {
    local os=$1
    case "$os" in
        linux)
            if command -v apt-get &> /dev/null; then
                echo "apt"
            elif command -v dnf &> /dev/null; then
                echo "dnf"
            elif command -v pacman &> /dev/null; then
                echo "pacman"
            else
                echo "unknown"
            fi
            ;;
        mac)
            if command -v brew &> /dev/null; then
                echo "brew"
            else
                echo "unknown"
            fi
            ;;
        *)  echo "unknown" ;;
    esac
}

install_deps() {
    local os=$1
    local pkg_manager=$2
    local use_mkl=${3:-false}
    
    case "$os" in
        linux)
            case "$pkg_manager" in
                apt)
                    sudo apt-get update
                    sudo apt-get install -y meson ninja-build libopenblas-dev libomp-dev liblapacke-dev
                    ;;
                dnf)
                    sudo dnf install -y meson ninja-build openblas-devel libomp-devel lapack-devel
                    ;;
                pacman)
                    sudo pacman -S --noconfirm meson ninja openblas libomp lapack
                    ;;
                *)
                    echo "Unknown package manager: $pkg_manager"
                    return 1
                    ;;
            esac
            ;;
        mac)
            case "$pkg_manager" in
                brew)
                    brew install meson ninja
                    brew install libomp || true
                    ;;
                *)
                    echo "Unknown package manager: $pkg_manager"
                    return 1
                    ;;
            esac
            ;;
        *)
            echo "Unknown OS: $os"
            return 1
            ;;
    esac
}

install_mkl() {
    local os=$1
    case "$os" in
        linux)
            if command -v apt-get &> /dev/null; then
                echo "Installing Intel MKL..."
                wget https://apt.repos.intel.com/oneapi/intel-oneapi-repo-1.0-1_all.deb
                sudo dpkg -i intel-oneapi-repo-1.0-1_all.deb
                sudo apt-get update
                sudo apt-get install -y intel-oneapi-mkl
            fi
            ;;
        mac)
            echo "MKL on macOS requires Intel Compiler. Consider using Apple Accelerate instead."
            ;;
    esac
}

check_deps() {
    local os=$1
    local missing=()
    
    if ! command -v meson &> /dev/null; then
        missing+=("meson")
    fi
    
    if ! command -v ninja &> /dev/null; then
        missing+=("ninja")
    fi
    
    case "$os" in
        linux)
            if ! pkg-config --exists openblas 2>/dev/null; then
                if [ ! -d "/usr/lib/x86_64-linux-gnu/libopenblas.so" ] && \
                   [ ! -d "/usr/lib/libopenblas.so" ]; then
                    missing+=("libopenblas-dev")
                fi
            fi
            if ! pkg-config --exists omp 2>/dev/null; then
                if ! ls /usr/lib/gcc/x86_64-linux-gnu/*/libgomp.so >/dev/null 2>&1; then
                    missing+=("libomp-dev")
                fi
            fi
            if ! pkg-config --exists lapacke 2>/dev/null; then
                if [ ! -f "/usr/include/lapacke.h" ] && [ ! -f "/usr/include/x86_64-linux-gnu/lapacke.h" ]; then
                    missing+=("liblapacke-dev")
                fi
            fi
            ;;
        mac)
            echo "macOS uses Apple Accelerate by default (no extra deps needed)"
            ;;
    esac
    
    if [ ${#missing[@]} -gt 0 ]; then
        echo "Missing dependencies: ${missing[*]}"
        return 1
    fi
    
    echo "All dependencies satisfied"
    return 0
}

setup_build() {
    local build_type=$1
    local build_dir="build/${build_type}"
    
    rm -rf "$build_dir"
    meson setup "$build_dir" -Dbuildtype="$build_type" -Dprefix="$(pwd)"
    
    if [ $? -ne 0 ]; then
        echo "Meson setup failed"
        return 1
    fi
    
    echo "Build directory configured: $build_dir"
}

compile() {
    local build_type=$1
    local build_dir="build/${build_type}"
    
    meson compile -C "$build_dir"
    
    if [ $? -ne 0 ]; then
        echo "Compilation failed"
        return 1
    fi
    
    echo "Compilation successful"
}

install() {
    local build_type=$1
    local build_dir="build/${build_type}"
    
    meson install -C "$build_dir"
    
    if [ $? -ne 0 ]; then
        echo "Installation failed"
        return 1
    fi
    
    echo "Installation successful"
}

run_tests() {
    local build_type=$1
    local build_dir="build/${build_type}"
    
    if [ ! -d "$build_dir" ]; then
        echo "Build directory not found. Run setup first."
        return 1
    fi
    
    meson test -C "$build_dir"
}

clean() {
    rm -rf build dist
    echo "Cleaned build and dist directories"
}

get_build_type() {
    if [ "$1" = "debug" ]; then
        echo "debug"
    else
        echo "release"
    fi
}
