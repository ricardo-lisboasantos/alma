#!/bin/bash

source "$(dirname "$0")/build-common.sh"

setup_android() {
    echo "Setting up Android NDK toolchain..."

    if [ -n "$ANDROID_NDK_ROOT" ] || [ -n "$ANDROID_NDK_HOME" ]; then
        echo "Android NDK found at: ${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}"
    else
        echo "Error: Android NDK not found."
        echo "Please set ANDROID_NDK_ROOT or ANDROID_NDK_HOME environment variable."
        echo ""
        echo "To install Android NDK:"
        echo "  1. Download from: https://developer.android.com/ndk/downloads"
        echo "  2. Extract to /opt/android-ndk or similar"
        echo "  3. Set: export ANDROID_NDK_ROOT=/opt/android-ndk"
        return 1
    fi

    echo "Android NDK path: ${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}"
    echo ""
    echo "To build for Android, run:"
    echo "  make android"
    echo ""
    echo "Or cross-compile with:"
    echo "  meson setup build/android \\"
    echo "    --cross-file=android-crossfile.txt \\"
    echo "    -Dbuildtype=release"
}

setup_ios() {
    echo "Setting up iOS toolchain..."

    if ! command -v xcodebuild &> /dev/null; then
        echo "Error: Xcode not found. Please install Xcode from App Store."
        return 1
    fi

    if ! command -v xcrun &> /dev/null; then
        echo "Error: xcrun not found. Please install Xcode command line tools."
        echo "Run: xcode-select --install"
        return 1
    fi

    echo "Xcode tools found"
    echo ""
    echo "To build for iOS, run:"
    echo "  make ios"
    echo ""
    echo "Or use:"
    echo "  make ios-sim"
}

setup_windows() {
    echo "Setting up Windows toolchain..."

    case "$(uname -s)" in
        Linux*)
            if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
                echo "MinGW-w64 not found. Install with:"
                echo "  sudo apt-get install mingw-w64"
                return 1
            fi
            echo "MinGW-w64 found"
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
            echo "Windows host detected - using MSVC or MinGW"
            ;;
    esac

    echo ""
    echo "To build for Windows, run:"
    echo "  make windows"
    echo ""
    echo "Or cross-compile with:"
    echo "  meson setup build/windows --cross-file=windows-crossfile.txt"
}

setup_linux() {
    echo "Setting up Linux toolchain..."
    local pkg_manager=""

    if command -v apt-get &> /dev/null; then
        pkg_manager="apt"
    elif command -v dnf &> /dev/null; then
        pkg_manager="dnf"
    elif command -v pacman &> /dev/null; then
        pkg_manager="pacman"
    else
        echo "No known package manager found (apt, dnf, pacman)"
        echo "Please install manually:"
        echo "  - meson, ninja"
        echo "  - libopenblas-dev, liblapacke-dev"
        echo "  - libomp-dev"
        return 1
    fi

    case "$pkg_manager" in
        apt)
            echo "Installing Linux build dependencies via apt..."
            sudo apt-get update
            sudo apt-get install -y \
                meson ninja-build \
                libopenblas-dev liblapacke-dev \
                libomp-dev cmake \
                pkg-config
            ;;
        dnf)
            echo "Installing Linux build dependencies via dnf..."
            sudo dnf install -y \
                meson ninja-build \
                openblas-devel lapack-devel \
                libomp-devel cmake \
                pkg-config
            ;;
        pacman)
            echo "Installing Linux build dependencies via pacman..."
            sudo pacman -S --noconfirm \
                meson ninja \
                openblas lapack \
                libomp cmake \
                pkgconf
            ;;
        *)
            echo "Unknown package manager: $pkg_manager"
            return 1
            ;;
    esac

    echo "Linux toolchain ready!"
}

setup_macos() {
    echo "Setting up macOS toolchain..."

    if ! command -v brew &> /dev/null; then
        echo "Error: Homebrew not found. Install from https://brew.sh"
        return 1
    fi

    echo "Installing macOS build dependencies..."
    brew install meson ninja libomp

    echo ""
    echo "Note: macOS uses Apple Accelerate framework for BLAS/LAPACK."
    echo "The following are optional:"
    echo "  brew install openblas  # For OpenBLAS (if needed)"
    echo "  brew install lapack   # For LAPACK headers (if needed)"

    echo ""
    echo "macOS toolchain ready!"
    echo "Build with: make release"
}

show_setup_help() {
    echo "Usage: make setup <target>"
    echo ""
    echo "Available targets:"
    echo "  linux   - Set up Linux build toolchain"
    echo "  macos   - Set up macOS build toolchain"
    echo "  android - Set up Android NDK toolchain"
    echo "  ios     - Set up iOS toolchain"
    echo "  windows - Set up Windows cross-compilation toolchain"
    echo ""
    echo "Examples:"
    echo "  make setup linux"
    echo "  make setup android"
    echo "  make setup ios"
}

TARGET=$1

case "$TARGET" in
    linux|linux*)
        setup_linux
        ;;
    macos|mac|darwin)
        setup_macos
        ;;
    android)
        setup_android
        ;;
    ios)
        setup_ios
        ;;
    windows|win)
        setup_windows
        ;;
    help|"")
        show_setup_help
        ;;
    *)
        echo "Unknown target: $TARGET"
        show_setup_help
        exit 1
        ;;
esac
