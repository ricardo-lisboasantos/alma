#!/usr/bin/env python3
import os
import random

DATA_DIR = os.path.dirname(os.path.abspath(__file__)) + "/data"


def generate_uniform_matrix(n, scale=1.0):
    """Generate an n x n matrix with uniform random values."""
    return [[random.uniform(-1, 1) * scale for _ in range(n)] for _ in range(n)]


def generate_identity_matrix(n):
    """Generate an n x n identity matrix."""
    return [[1.0 if i == j else 0.0 for j in range(n)] for i in range(n)]


def generate_sparse_matrix(n, density=0.1, scale=1.0):
    """Generate an n x n sparse matrix with given density."""
    return [
        [
            random.uniform(-1, 1) * scale if random.random() < density else 0.0
            for _ in range(n)
        ]
        for _ in range(n)
    ]


def generate_diagonal_matrix(n, scale=1.0):
    """Generate an n x n diagonal matrix."""
    return [[(i + 1) * scale if i == j else 0.0 for j in range(n)] for i in range(n)]


def generate_banded_matrix(n, bandwidth=2, scale=1.0):
    """Generate an n x n banded matrix with given bandwidth."""
    return [
        [(n - abs(i - j)) * scale if abs(i - j) <= bandwidth else 0.0 for j in range(n)]
        for i in range(n)
    ]


def generate_toeplitz_matrix(n, scale=1.0):
    """Generate an n x n Toeplitz matrix."""
    return [[(1.0 / (abs(i - j) + 1)) * scale for j in range(n)] for i in range(n)]


def generate_hilbert_matrix(n, scale=1.0):
    """Generate an n x n Hilbert matrix."""
    return [[(1.0 / (i + j + 1)) * scale for j in range(n)] for i in range(n)]


def generate_ones_matrix(n, scale=1.0):
    """Generate an n x n matrix filled with ones."""
    return [[scale for _ in range(n)] for _ in range(n)]


def generate_matrix(n, filename, pattern="random", scale=1.0):
    """Generate an n x n matrix and save to CSV."""
    filepath = os.path.join(DATA_DIR, filename)
    with open(filepath, "w") as f:
        match pattern:
            case "random":
                matrix = generate_uniform_matrix(n, scale)
            case "identity":
                matrix = generate_identity_matrix(n)
            case "sparse":
                matrix = generate_sparse_matrix(n, density=0.1, scale=scale)
            case "diagonal":
                matrix = generate_diagonal_matrix(n, scale)
            case "banded":
                matrix = generate_banded_matrix(n, bandwidth=2, scale=scale)
            case "toeplitz":
                matrix = generate_toeplitz_matrix(n, scale)
            case "hilbert":
                matrix = generate_hilbert_matrix(n, scale)
            case "ones":
                matrix = generate_ones_matrix(n, scale)
            case _:
                raise ValueError(f"Unknown pattern: {pattern}")
        for row in matrix:
            f.write(",".join(f"{val:.6f}" for val in row) + "\n")
    print(f"Generated {filepath}: {n}x{n} {pattern}")


if __name__ == "__main__":
    # Generate various test matrices
    generate_matrix(512, "matrix1.csv", "random")
    generate_matrix(512, "matrix2.csv", "random")

    generate_matrix(1024, "matrix_1024_random.csv", "random")
    generate_matrix(1024, "matrix_1024_sparse.csv", "sparse")
    generate_matrix(1024, "matrix_1024_identity.csv", "identity")
    generate_matrix(1024, "matrix_1024_banded.csv", "banded")

    generate_matrix(2048, "matrix_2048_random.csv", "random")
    generate_matrix(2048, "matrix_2048_sparse.csv", "sparse")
    generate_matrix(2048, "matrix_2048_identity.csv", "identity")
    generate_matrix(2048, "matrix_2048_banded.csv", "banded")

    generate_matrix(4096, "matrix_4096_random.csv", "random")
    generate_matrix(4096, "matrix_4096_sparse.csv", "sparse")
    generate_matrix(4096, "matrix_4096_identity.csv", "identity")

    print("Done!")
