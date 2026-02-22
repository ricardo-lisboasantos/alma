# Simple Makefile for building the alma example using OpenBLAS
# Usage:
#   make        # builds ./alma
#   make run    # builds and runs
#   make clean  # removes binary

CXX := g++-15
SRC := src/main.cpp
LIBSRC := src/alma.cpp
BIN := dist/alma
TESTBIN := tests/test_alma
BENCHBIN := bench/benchmark
QUICKBENCHBIN := bench/quick_bench

# Try pkg-config first (works if openblas provides a .pc), otherwise
# try common Homebrew locations, otherwise fall back to -lopenblas and hope
PKG_CFLAGS := $(shell pkg-config --cflags openblas 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs openblas 2>/dev/null)

CXXFLAGS := -O3 -std=c++17 -fopenmp -march=native -ffast-math $(PKG_CFLAGS)
LDFLAGS  := -L/opt/homebrew/opt/libomp/lib -lomp $(PKG_LIBS)

ifeq ($(strip $(PKG_CFLAGS)$(PKG_LIBS)),)
  ifneq (,$(wildcard /opt/homebrew/opt/openblas/include))
    CXXFLAGS += -I/opt/homebrew/opt/openblas/include
    LDFLAGS  += -L/opt/homebrew/opt/openblas/lib -lopenblas
  else ifneq (,$(wildcard /usr/local/opt/openblas/include))
    CXXFLAGS += -I/usr/local/opt/openblas/include
    LDFLAGS  += -L/usr/local/opt/openblas/lib -lopenblas
  else
    # Last resort: rely on system include/search paths and link name
    LDFLAGS += -lopenblas
  endif
endif

.PHONY: all run test clean
all: $(BIN)

$(BIN): $(SRC) $(LIBSRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TESTBIN): tests/test_alma.cpp $(LIBSRC)
	$(CXX) $(CXXFLAGS) -I./src -o $@ $^ $(LDFLAGS)

$(BENCHBIN): bench/benchmark.cpp $(LIBSRC)
	$(CXX) $(CXXFLAGS) -I./src -o $@ $^ $(LDFLAGS)

$(QUICKBENCHBIN): bench/quick_bench.cpp $(LIBSRC)
	$(CXX) $(CXXFLAGS) -I./src -o $@ $^ $(LDFLAGS)

run: $(BIN)
	./$(BIN)

test: $(TESTBIN)
	./$(TESTBIN)

bench: $(BENCHBIN)
	./$(BENCHBIN) $(n) $(block) 3

quick: $(QUICKBENCHBIN)
	./$(QUICKBENCHBIN) $(n) $(block) $(repeats)

clean:
	rm -f $(BIN) $(TESTBIN) $(BENCHBIN) $(QUICKBENCHBIN)

# Helpful notes:
# - On macOS with Homebrew: `brew install openblas` (Homebrew puts it in
#   /opt/homebrew/opt/openblas or /usr/local/opt/openblas depending on CPU).
# - On Debian/Ubuntu: `sudo apt-get install libopenblas-dev` (provides pkg-config
#   or at least the library name `openblas`).
# - If OpenBLAS is in a custom prefix, set PKG_CONFIG_PATH or use:
#     make CXXFLAGS="-I/path/to/openblas/include" LDFLAGS="-L/path/to/openblas/lib -lopenblas"
