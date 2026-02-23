#!/bin/bash

source "$(dirname "$0")/build-common.sh"

detect_openblas() {
    local openblas_paths=(
        "/opt/homebrew/opt/openblas"
        "/usr/local/opt/openblas"
        "/opt/local/opt/openblas"
    )
    
    for path in "${openblas_paths[@]}"; do
        if [ -d "$path" ]; then
            echo "$path"
            return 0
        fi
    done
    
    if command -v brew &> /dev/null; then
        local brew_prefix
        brew_prefix=$(brew --prefix openblas 2>/dev/null || echo "")
        if [ -n "$brew_prefix" ] && [ -d "$brew_prefix" ]; then
            echo "$brew_prefix"
            return 0
        fi
    fi
    
    return 1
}

get_mac_deps() {
    echo "macOS dependencies:"
    echo "  - meson: brew install meson"
    echo "  - ninja: brew install ninja"
    echo "  - openblas: brew install openblas"
    echo "  - libomp: brew install libomp"
    echo ""
    echo "Full install: brew install meson ninja openblas libomp"
}
