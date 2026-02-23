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

check_android_deps() {
    if [ -z "$ANDROID_NDK_ROOT" ] && [ -z "$ANDROID_NDK_HOME" ]; then
        echo "Error: Android NDK not found."
        echo "Please set ANDROID_NDK_ROOT or ANDROID_NDK_HOME environment variable."
        echo ""
        echo "To install Android NDK:"
        echo "  1. Download from: https://developer.android.com/ndk/downloads"
        echo "  2. Extract to /opt/android-ndk or similar"
        echo "  3. Set: export ANDROID_NDK_ROOT=/opt/android-ndk"
        return 1
    fi
    
    local ndk_path="${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}"
    echo "Android NDK found at: $ndk_path"
    
    if [ ! -d "$ndk_path/toolchains/llvm/prebuilt" ]; then
        echo "Error: LLVM toolchain not found in NDK"
        return 1
    fi
    
    if ! command -v meson &> /dev/null; then
        echo "Error: meson not found. Install with: pip3 install meson"
        return 1
    fi
    
    if ! command -v ninja &> /dev/null; then
        echo "Error: ninja not found. Install with: pip3 install ninja"
        return 1
    fi
    
    echo "Android build dependencies satisfied"
    return 0
}

get_android_toolchain_path() {
    local ndk_path="${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}"
    echo "$ndk_path/toolchains/llvm/prebuilt/linux-x86_64/bin"
}

setup_android_build() {
    local build_type=${1:-release}
    local build_dir="build/android"
    local cross_file="android-crossfile.txt"
    
    if [ ! -f "$cross_file" ]; then
        echo "Error: Cross-file $cross_file not found"
        return 1
    fi
    
    local ndk_path="${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}"
    local toolchain_path=$(get_android_toolchain_path)
    local sysroot="$ndk_path/toolchains/llvm/prebuilt/linux-x86_64/sysroot"
    
    rm -rf "$build_dir"
    mkdir -p "$build_dir"
    
    local cross_content=$(cat "$cross_file")
    cross_content="${cross_content//ANDROID_NDK_PATH/$ndk_path}"
    cross_content="${cross_content//TOOLCHAIN_PATH/$toolchain_path}"
    cross_content="${cross_content//SYSROOT_PATH/$sysroot}"
    
    echo "$cross_content" > "$build_dir/cross-file.txt"
    
    meson setup "$build_dir" \
        --cross-file="$build_dir/cross-file.txt" \
        -Dbuildtype="$build_type" \
        -Dstatic-link=true \
        -Dprefix="$(pwd)"
    
    if [ $? -ne 0 ]; then
        echo "Meson setup failed"
        return 1
    fi
    
    echo "Android build directory configured: $build_dir"
}

compile_android() {
    local build_type=${1:-release}
    local build_dir="build/android"
    
    if [ ! -d "$build_dir" ]; then
        echo "Build directory not found. Run setup first."
        return 1
    fi
    
    meson compile -C "$build_dir"
    
    if [ $? -ne 0 ]; then
        echo "Android compilation failed"
        return 1
    fi
    
    echo "Android compilation successful"
}

install_android() {
    local build_type=${1:-release}
    local build_dir="build/android"
    
    if [ ! -d "$build_dir" ]; then
        echo "Build directory not found. Run setup first."
        return 1
    fi
    
    meson install -C "$build_dir"
    
    if [ $? -ne 0 ]; then
        echo "Android installation failed"
        return 1
    fi
    
    echo "Android installation successful"
}

build_blis_for_android() {
    local ndk_path="${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}"
    local install_dir="$(pwd)/vendor/blis/android-arm64"
    
    if [ -d "$install_dir/lib" ] && [ -f "$install_dir/lib/libblis.a" ]; then
        echo "BLIS already built at $install_dir"
        return 0
    fi
    
    echo "Building BLIS for Android arm64..."
    
    local blis_src="/tmp/blis-src"
    if [ ! -d "$blis_src/.git" ]; then
        echo "Cloning BLIS..."
        rm -rf "$blis_src"
        git clone --depth 1 https://github.com/flame/blis.git "$blis_src"
    fi
    
    cd "$blis_src"
    
    local toolchain="$ndk_path/toolchains/llvm/prebuilt/linux-x86_64"
    local cc="$toolchain/bin/aarch64-linux-android21-clang"
    local cxx="$toolchain/bin/aarch64-linux-android21-clang++"
    local ar="$toolchain/bin/llvm-ar"
    local ranlib="$toolchain/bin/llvm-ranlib"
    
    CC="$cc" \
    CXX="$cxx" \
    AR="$ar" \
    RANLIB="$ranlib" \
    CFLAGS="-O3 -march=armv8.2-a+dotprod -fPIC -DANDROID -D__ANDROID__" \
    LDFLAGS="-lm" \
    ./configure \
        --prefix="$install_dir" \
        --disable-shared \
        --enable-static \
        --enable-blas \
        --enable-cblas \
        --enable-lapack \
        --enable-lapacke \
        --disable-threading \
        --disable-system \
        arm64
    
    make -j$(nproc)
    make install
    
    cd -
    
    mkdir -p "$install_dir/lib/pkgconfig"
    
    cat > "$install_dir/lib/pkgconfig/blis.pc" << EOF
prefix=$install_dir
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: BLIS
Description: BLAS-like Library Instantiation Software
Version: 3.0
Libs: -L\${libdir} -lblis
Cflags: -I\${includedir}
EOF

    echo "BLIS with LAPACKE installed to $install_dir"
}

build_lapack_for_android() {
    local ndk_path="${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}"
    local install_dir="$(pwd)/vendor/lapack/android-arm64"
    local blis_dir="$(pwd)/vendor/blis/android-arm64"
    
    if [ -d "$install_dir/lib" ] && [ -f "$install_dir/lib/liblapack.a" ]; then
        echo "LAPACK already built at $install_dir"
        return 0
    fi
    
    echo "Building LAPACK for Android arm64..."
    
    local lapack_src="/tmp/lapack-src"
    if [ ! -d "$lapack_src/.git" ]; then
        echo "Cloning LAPACK..."
        rm -rf "$lapack_src"
        git clone --depth 1 https://github.com/Reference-LAPACK/lapack.git "$lapack_src"
    fi
    
    cd "$lapack_src"
    
    local toolchain="$ndk_path/toolchains/llvm/prebuilt/linux-x86_64"
    local cc="$toolchain/bin/aarch64-linux-android21-clang"
    local ar="$toolchain/bin/llvm-ar"
    local ranlib="$toolchain/bin/llvm-ranlib"
    
    cp ../lapack.pc.in ../lapack.pc
    
    cat > make.inc << EOF
SHELL = /bin/sh
CC = $cc
AR = $ar
ARFLAGS = r
RANLIB = $ranlib
BLASLIB = $blis_dir/lib/libblis.a
LAPACKLIB = liblapack.a
TMGLIB = libtmglib.a
LAPACKELIB = liblapacke.a
CBLASLIB = libcblas.a

CFLAGS = -O3 -fPIC -march=armv8.2-a+dotprod -DANDROID -D__ANDROID__
NOOPT = -O3 -fPIC -march=armv8.2-a+dotprod -DANDROID -D__ANDROID__
PIC = -fPIC

BUILD = $cc
LOADER = $cc

MAKE = make
EOF
    
    mkdir -p lib
    $ar cr lib/liblapack.a lapack_lite/src/*.o
    $ar cr lib/liblapacke.a SRC/lapacke/*.o
    $ranlib lib/liblapack.a
    $ranlib lib/liblapacke.a
    
    mkdir -p "$install_dir/lib"
    cp lib/liblapack.a "$install_dir/lib/"
    cp lib/liblapacke.a "$install_dir/lib/"
    
    mkdir -p "$install_dir/include"
    cp -r SRC/lapacke.h "$install_dir/include/" 2>/dev/null || true
    cp -r INCLUDE/*.h "$install_dir/include/" 2>/dev/null || true
    
    cd -
    
    echo "LAPACK installed to $install_dir"
}

check_windows_deps() {
    case "$(uname -s)" in
        Linux*)
            if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
                echo "Error: MinGW-w64 not found. Install with:"
                echo "  sudo apt-get install mingw-w64"
                return 1
            fi
            ;;
        Darwin*)
            if ! command -v brew &> /dev/null; then
                echo "Error: Homebrew not found."
                return 1
            fi
            if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
                echo "Installing MinGW-w64..."
                brew install mingw-w64
            fi
            ;;
        CYGWIN*|MINGW*)
            echo "Windows host detected - using native compiler"
            ;;
    esac
    
    if ! command -v meson &> /dev/null; then
        echo "Error: meson not found. Install with: pip3 install meson"
        return 1
    fi
    
    if ! command -v ninja &> /dev/null; then
        echo "Error: ninja not found. Install with: pip3 install ninja"
        return 1
    fi
    
    echo "Windows cross-compile dependencies satisfied"
    return 0
}

setup_windows_build() {
    local build_type=${1:-release}
    local build_dir="build/windows"
    local cross_file="windows-crossfile.txt"
    
    if [ ! -f "$cross_file" ]; then
        echo "Error: Cross-file $cross_file not found"
        return 1
    fi
    
    rm -rf "$build_dir"
    meson setup "$build_dir" \
        --cross-file="$cross_file" \
        -Dbuildtype="$build_type" \
        -Dprefix="$(pwd)"
    
    if [ $? -ne 0 ]; then
        echo "Meson setup failed"
        return 1
    fi
    
    echo "Windows build directory configured: $build_dir"
}

compile_windows() {
    local build_type=${1:-release}
    local build_dir="build/windows"
    
    if [ ! -d "$build_dir" ]; then
        echo "Build directory not found. Run setup first."
        return 1
    fi
    
    meson compile -C "$build_dir"
    
    if [ $? -ne 0 ]; then
        echo "Windows compilation failed"
        return 1
    fi
    
    echo "Windows compilation successful"
}

install_windows() {
    local build_type=${1:-release}
    local build_dir="build/windows"
    
    if [ ! -d "$build_dir" ]; then
        echo "Build directory not found. Run setup first."
        return 1
    fi
    
    meson install -C "$build_dir"
    
    if [ $? -ne 0 ]; then
        echo "Windows installation failed"
        return 1
    fi
    
    echo "Windows installation successful"
}

check_ios_deps() {
    if ! command -v xcodebuild &> /dev/null; then
        echo "Error: Xcode not found. Please install Xcode from App Store."
        return 1
    fi
    
    if ! command -v xcrun &> /dev/null; then
        echo "Error: xcrun not found. Please install Xcode command line tools."
        echo "Run: xcode-select --install"
        return 1
    fi
    
    if ! command -v meson &> /dev/null; then
        echo "Error: meson not found. Install with: pip3 install meson"
        return 1
    fi
    
    if ! command -v ninja &> /dev/null; then
        echo "Error: ninja not found. Install with: pip3 install ninja"
        return 1
    fi
    
    echo "iOS build dependencies satisfied"
    return 0
}

setup_ios_build() {
    local build_type=${1:-release}
    local build_dir="build/ios"
    local cross_file="ios-crossfile.txt"
    
    if [ ! -f "$cross_file" ]; then
        echo "Error: Cross-file $cross_file not found"
        return 1
    fi
    
    rm -rf "$build_dir"
    meson setup "$build_dir" \
        --cross-file="$cross_file" \
        -Dbuildtype="$build_type" \
        -Dprefix="$(pwd)"
    
    if [ $? -ne 0 ]; then
        echo "Meson setup failed"
        return 1
    fi
    
    echo "iOS build directory configured: $build_dir"
}

compile_ios() {
    local build_type=${1:-release}
    local build_dir="build/ios"
    
    if [ ! -d "$build_dir" ]; then
        echo "Build directory not found. Run setup first."
        return 1
    fi
    
    meson compile -C "$build_dir"
    
    if [ $? -ne 0 ]; then
        echo "iOS compilation failed"
        return 1
    fi
    
    echo "iOS compilation successful"
}

install_ios() {
    local build_type=${1:-release}
    local build_dir="build/ios"
    
    if [ ! -d "$build_dir" ]; then
        echo "Build directory not found. Run setup first."
        return 1
    fi
    
    meson install -C "$build_dir"
    
    if [ $? -ne 0 ]; then
        echo "iOS installation failed"
        return 1
    fi
    
    echo "iOS installation successful"
}
