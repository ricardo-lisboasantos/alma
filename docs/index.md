# ALMA Documentation

Fast blocked matrix multiplication with low-rank detection.

## Quick Start

```bash
brew install openblas
make
make test
make bench
```

See [README.md](../README.md) for full build instructions.

## Documentation

| Guide | Description |
|-------|-------------|
| [High-Level Design](high_level.md) | System architecture, components, data flow |
| [Low-Level Design](low_level.md) | Data structures, algorithms, implementation details |
| [Architecture](architecture.md) | System design, components, API reference |
| [Algorithm](paper.md) | Mathematical background and complexity |
| [Performance](performance.md) | Benchmarking, tuning, optimization |
| [Contributing](contributing.md) | Development setup, code style |

## Key Topics

- [System architecture](high_level.md) - Three-layer design
- [Block classification](low_level.md#classify_block) - SVD-based analysis
- [API functions](architecture.md#api-reference)
- [Algorithm complexity](paper.md#complexity-analysis)
- [Tuning block size](performance.md#tuning-parameters)
- [CSV benchmarks](performance.md#csv-benchmark-matrices)

## Diagrams

See diagram sources in `docs/diagrams/`:
- [HLD Diagrams](diagrams/hld.mmd) - High-level system architecture
- [LLD Diagrams](diagrams/lld.mmd) - Low-level implementation details
- [Architecture](diagrams/architecture.mmd) - Algorithm flow, class, sequence diagrams

## Test Suite

28 tests across 7 categories:

```bash
make test
```

## Benchmarks

```bash
make bench                    # Default benchmark
./bench/benchmark --csv-bench # CSV matrix benchmarks
./bench/benchmark --sweep    # Size/block sweep
```
