#!/bin/bash

case "$(uname -s)" in
    Linux*)
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
    Darwin*)
        if command -v brew &> /dev/null; then
            echo "brew"
        else
            echo "unknown"
        fi
        ;;
    *)
        echo "unknown"
        ;;
esac
