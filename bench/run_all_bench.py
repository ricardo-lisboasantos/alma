# excutes several benchmarks and aggregates the results in a single printout
import subprocess


# run `./bench/benchmark -a bench/data/matrix1.csv -B bench/data/matrix2.csv -s 2048 -b 120`
def run_bench(
    command=[
        "./bench/benchmark",
        "-a",
        "bench/data/matrix1.csv",
        "-B",
        "bench/data/matrix2.csv",
        "-s",
        "2048",
        "-b",
        "120",
    ],
):
    result = subprocess.run(
        command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    if result.returncode != 0:
        print(f"Error running command: {' '.join(command)}")
        print(f"Error message: {result.stderr}")
        return None
    return result.stdout.strip()


def main():
    data_files = [
        ("bench/data/matrix1.csv", "bench/data/matrix2.csv"),
        ("bench/data/matrix_1024_random.csv", "bench/data/matrix_1024_random.csv"),
        ("bench/data/matrix_1024_sparse.csv", "bench/data/matrix_1024_sparse.csv"),
        ("bench/data/matrix_1024_identity.csv", "bench/data/matrix_1024_identity.csv"),
        ("bench/data/matrix_1024_banded.csv", "bench/data/matrix_1024_banded.csv"),
        ("bench/data/matrix_2048_random.csv", "bench/data/matrix_2048_random.csv"),
    ]

    results = []
    for a, b in data_files:
        print(f"Running benchmark for {a} and {b}...")
        output = run_bench(
            ["./bench/benchmark", "-a", a, "-B", b, "-s", "2048", "-b", "120"]
        )
        if output is not None:
            results.append((a, b, output))

    print("\nBenchmark Results:")
    for a, b, output in results:
        print(f"{a} x {b}: {output}")


if __name__ == "__main__":
    main()
