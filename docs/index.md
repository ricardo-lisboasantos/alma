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
| [Architecture](architecture.md) | System design, components, API reference |
| [Algorithm](paper.md) | Mathematical background and complexity |
| [Performance](performance.md) | Benchmarking, tuning, optimization |
| [Contributing](contributing.md) | Development setup, code style |

## Key Topics

- [Block classification](architecture.md#blocktype)
- [API functions](architecture.md#api-reference)
- [Algorithm complexity](paper.md#complexity-analysis)
- [Tuning block size](performance.md#tuning-parameters)
- [CSV benchmarks](performance.md#csv-benchmark-matrices)

## Diagrams

See [diagrams/architecture.mmd](../docs/diagrams/architecture.mmd) for source:
- Flowchart: Algorithm control flow
- Class diagram: Data structures
- Sequence diagram: Execution model

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
