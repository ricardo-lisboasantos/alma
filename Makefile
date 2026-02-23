# Simple Makefile for building the alma example using OpenBLAS

# Auto-detect best available compiler
ifndef CXX
  CXX := $(shell (command -v g++-15 || command -v g++-14 || command -v g++-13 || command -v g++ || echo "g++") 2>/dev/null)
endif

# Detect if using clang
IS_CLANG := $(findstring clang,$(shell $(CXX) --version 2>/dev/null | head -1 | tr '[:upper:]' '[:lower:]'))

# OpenMP flags - use libomp for clang
OPENMP := -fopenmp
OPENMP_LDFLAGS :=
OPENMP_CFLAGS :=
ifneq ($(findstring clang,$(IS_CLANG)),)
  # clang needs -Xpreprocessor -fopenmp -lomp (passes -lomp to linker)
  # but we also need to add library search path
  OPENMP := -Xpreprocessor -fopenmp -lomp
  OPENMP_LDFLAGS := -L/opt/homebrew/opt/libomp/lib
  OPENMP_CFLAGS := -I/opt/homebrew/opt/libomp/include
endif

SRC := src/main.cpp
LIBSRC := src/alma.cpp
BIN := dist/alma
TESTBIN := tests/test_alma
BENCHBIN := bench/benchmark
QUICKBENCHBIN := bench/quick_bench
TESTSRC := tests/test_alma.cpp tests/test_util.cpp tests/test_basic.cpp tests/test_edge.cpp tests/test_random.cpp tests/test_special.cpp tests/test_large.cpp tests/test_csv.cpp

PKG_CFLAGS := $(shell pkg-config --cflags openblas 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs openblas 2>/dev/null)

CXXFLAGS := -O3 -std=c++17 $(OPENMP) -march=native -ffast-math $(PKG_CFLAGS) $(OPENMP_CFLAGS)
LDFLAGS  := $(PKG_LIBS)

ifeq ($(strip $(PKG_CFLAGS)$(PKG_LIBS)),)
  ifneq (,$(wildcard /opt/homebrew/opt/openblas/include))
    CXXFLAGS += -I/opt/homebrew/opt/openblas/include
    LDFLAGS  += -L/opt/homebrew/opt/openblas/lib -lopenblas $(OPENMP_LDFLAGS)
  else ifneq (,$(wildcard /usr/local/opt/openblas/include))
    CXXFLAGS += -I/usr/local/opt/openblas/include
    LDFLAGS  += -L/usr/local/opt/openblas/lib -lopenblas
  else
    LDFLAGS += -lopenblas $(OPENMP_LDFLAGS)
  endif
endif

.PHONY: all run test clean bench quick
all: $(BIN)

$(BIN): $(SRC) $(LIBSRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TESTBIN): $(TESTSRC) $(LIBSRC)
	$(CXX) $(CXXFLAGS) -I./src -o $@ $(TESTSRC) $(LIBSRC) $(LDFLAGS)

$(BENCHBIN): bench/benchmark.cpp $(LIBSRC)
	$(CXX) $(CXXFLAGS) -I./src -o $@ $^ $(LDFLAGS)

$(QUICKBENCHBIN): bench/quick_bench.cpp $(LIBSRC)
	$(CXX) $(CXXFLAGS) -I./src -o $@ $^ $(LDFLAGS)

run: $(BIN)
	./$(BIN)

test: $(TESTBIN)
	./$(TESTBIN)

bench: $(BENCHBIN)
	./$(BENCHBIN) -s $(or $(n),1024) -b $(or $(block),128) -r 3

quick: $(QUICKBENCHBIN)
	./$(QUICKBENCHBIN) $(n) $(block) $(repeats)

clean:
	rm -f $(BIN) $(TESTBIN) $(BENCHBIN) $(QUICKBENCHBIN)
