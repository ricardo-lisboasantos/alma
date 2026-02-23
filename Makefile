.PHONY: all debug release test clean install deps check help setup android build-blis-android build-lapack-android

SCRIPT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
COMMON_SCRIPT := $(SCRIPT_DIR)/scripts/build-common.sh
SETUP_SCRIPT := $(SCRIPT_DIR)/scripts/build-setup.sh

OS := $(shell uname -s | tr '[:upper:]' '[:lower:]' | sed 's/darwin/mac/')

ifeq ($(OS),linux)
  PKG_MGR := $(shell if command -v apt-get &> /dev/null; then echo "apt"; else if command -v dnf &> /dev/null; then echo "dnf"; else echo "unknown"; fi; fi)
else ifeq ($(OS),mac)
  PKG_MGR := $(shell if command -v brew &> /dev/null; then echo "brew"; else echo "unknown"; fi)
else
  PKG_MGR := unknown
endif

define run_script
	@bash -c 'source $(COMMON_SCRIPT); $(1) $(2)'
endef

all: release

debug:
	@bash -c 'source $(COMMON_SCRIPT); check_deps $(OS)' || (echo "Installing dependencies..." && bash -c 'source $(COMMON_SCRIPT); install_deps $(OS) $(PKG_MGR)')
	@bash -c 'source $(COMMON_SCRIPT); setup_build debug'
	@bash -c 'source $(COMMON_SCRIPT); compile debug'
	@bash -c 'source $(COMMON_SCRIPT); install debug'

release:
	@bash -c 'source $(COMMON_SCRIPT); check_deps $(OS)' || (echo "Installing dependencies..." && bash -c 'source $(COMMON_SCRIPT); install_deps $(OS) $(PKG_MGR)')
	@bash -c 'source $(COMMON_SCRIPT); setup_build release'
	@bash -c 'source $(COMMON_SCRIPT); compile release'
	@bash -c 'source $(COMMON_SCRIPT); install release'

test: release
	@bash -c 'source $(COMMON_SCRIPT); run_tests release'

clean:
	@bash -c 'source $(COMMON_SCRIPT); clean'

install: release

deps:
	@bash -c 'source $(COMMON_SCRIPT); install_deps $(OS) $(PKG_MGR)'

check:
	@bash -c 'source $(COMMON_SCRIPT); check_deps $(OS)'

setup: setup-linux setup-macos setup-android setup-ios setup-windows

setup-linux:
	@bash $(SETUP_SCRIPT) linux

setup-macos:
	@bash $(SETUP_SCRIPT) macos

setup-android:
	@bash $(SETUP_SCRIPT) android

setup-ios:
	@bash $(SETUP_SCRIPT) ios

setup-windows:
	@bash $(SETUP_SCRIPT) windows

build-blis-android:
	@bash -c 'source $(COMMON_SCRIPT); check_android_deps' || (echo "Please set up Android NDK first" && exit 1)
	@bash -c 'source $(COMMON_SCRIPT); build_blis_for_android'

build-lapack-android:
	@bash -c 'source $(COMMON_SCRIPT); check_android_deps' || (echo "Please set up Android NDK first" && exit 1)
	@bash -c 'source $(COMMON_SCRIPT); build_lapack_for_android'

android:
	@bash -c 'source $(COMMON_SCRIPT); check_android_deps' || (echo "Please set up Android NDK first" && exit 1)
	@bash -c 'source $(COMMON_SCRIPT); build_blis_for_android'
	@bash -c 'source $(COMMON_SCRIPT); build_lapack_for_android'
	@bash -c 'source $(COMMON_SCRIPT); setup_android_build release'
	@bash -c 'source $(COMMON_SCRIPT); compile_android release'
	@bash -c 'source $(COMMON_SCRIPT); install_android release'

windows:
	@bash -c 'source $(COMMON_SCRIPT); check_windows_deps' || (echo "Please set up Windows cross-compile toolchain first" && exit 1)
	@bash -c 'source $(COMMON_SCRIPT); setup_windows_build release'
	@bash -c 'source $(COMMON_SCRIPT); compile_windows release'
	@bash -c 'source $(COMMON_SCRIPT); install_windows release'

ios:
	@bash -c 'source $(COMMON_SCRIPT); check_ios_deps' || (echo "Please set up iOS toolchain first" && exit 1)
	@bash -c 'source $(COMMON_SCRIPT); setup_ios_build release'
	@bash -c 'source $(COMMON_SCRIPT); compile_ios release'
	@bash -c 'source $(COMMON_SCRIPT); install_ios release'

help:
	@echo "Alma build system - Cross-platform Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build release version (default)"
	@echo "  debug     - Build debug version"
	@echo "  release   - Build release version"
	@echo "  test      - Run tests"
	@echo "  clean     - Clean build artifacts"
	@echo "  install   - Install built artifacts"
	@echo "  deps      - Install system dependencies"
	@echo "  check     - Check for missing dependencies"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Cross-compile targets:"
	@echo "  build-blis-android   - Build BLIS for Android arm64"
	@echo "  build-lapack-android - Build LAPACK for Android arm64"
	@echo "  android              - Build for Android arm64 (requires NDK)"
	@echo "  ios                  - Build for iOS (requires Xcode)"
	@echo "  windows              - Build for Windows (requires MinGW)"
	@echo ""
	@echo "Setup targets (install toolchains):"
	@echo "  make setup-linux    - Set up Linux build dependencies"
	@echo "  make setup-macos    - Set up macOS build dependencies"
	@echo "  make setup-android  - Set up Android NDK toolchain"
	@echo "  make setup-ios      - Set up iOS toolchain"
	@echo "  make setup-windows  - Set up Windows cross-compile toolchain"
	@echo ""
	@echo "Detected OS: $(OS)"
	@echo "Package manager: $(PKG_MGR)"
	@echo ""
	@if [ "$(OS)" = "linux" ]; then \
		$(SCRIPT_DIR)/scripts/build-linux.sh get_linux_deps; \
	elif [ "$(OS)" = "mac" ]; then \
		$(SCRIPT_DIR)/scripts/build-mac.sh get_mac_deps; \
	fi
