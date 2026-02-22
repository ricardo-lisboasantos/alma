# Simple Makefile for building the alma example using OpenBLAS

CXX := g++-15
SRC := src/main.cpp
LIBSRC := src/alma.cpp
BIN := dist/alma
TESTBIN := tests/test_alma
BENCHBIN := bench/benchmark
QUICKBENCHBIN := bench/quick_bench
TESTSRC := tests/test_alma.cpp tests/test_util.cpp tests/test_basic.cpp tests/test_edge.cpp tests/test_random.cpp tests/test_special.cpp tests/test_large.cpp tests/test_csv.cpp

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
    LDFLAGS += -lopenblas
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
