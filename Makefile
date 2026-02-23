.PHONY: all debug release test clean install deps check help

SCRIPT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
COMMON_SCRIPT := $(SCRIPT_DIR)/scripts/build-common.sh

OS := $(shell uname -s | tr '[:upper:]' '[:lower:]' | sed 's/darwin/mac/')

ifeq ($(OS),linux)
  PKG_MGR := $(shell if command -v apt-get &> /dev/null; then echo "apt"; else echo "unknown"; fi)
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

help:
	@echo "Alma build system - Cross-platform Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build release version (default)"
	@echo "  debug    - Build debug version"
	@echo "  release  - Build release version"
	@echo "  test     - Run tests"
	@echo "  clean    - Clean build artifacts"
	@echo "  install  - Install built artifacts"
	@echo "  deps     - Install system dependencies"
	@echo "  check    - Check for missing dependencies"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Detected OS: $(OS)"
	@echo "Package manager: $(PKG_MGR)"
	@echo ""
	@if [ "$(OS)" = "linux" ]; then \
		$(SCRIPT_DIR)/scripts/build-linux.sh get_linux_deps; \
	elif [ "$(OS)" = "mac" ]; then \
		$(SCRIPT_DIR)/scripts/build-mac.sh get_mac_deps; \
	fi
