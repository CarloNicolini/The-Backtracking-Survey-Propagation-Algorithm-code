#!/usr/bin/env python3
"""Exact small-N test of critical-clause and supported-variable biases."""

import argparse
import csv
import math
import random
import time
from collections import deque
from pathlib import Path


T_VALUES = (1.0, 0.95, 0.9, 0.8)
B_VALUES = (1.0, 0.8, 0.6)


def make_formula(k, n, alpha, seed):
    rng = random.Random(seed)
    clauses = []
    for _ in range(math.ceil(alpha * n)):
        variables = rng.sample(range(n), k)
        positive = 0
        negative = 0
        for variable in variables:
            if rng.getrandbits(1):
                positive |= 1 << variable
            else:
                negative |= 1 << variable
        clauses.append((positive, negative, variables))
    return clauses


def whitening_statistics(assignment, clauses, n, full_mask):
    critical = []
    supported = [0] * n
    for positive, negative, variables in clauses:
        satisfying = (assignment & positive) | ((full_mask ^ assignment) & negative)
        if satisfying and satisfying & (satisfying - 1) == 0:
            supporter = satisfying.bit_length() - 1
            critical.append((supporter, variables))
            supported[supporter] += 1

    c1 = len(critical)
    v1 = sum(count > 0 for count in supported)
    incident = [[] for _ in range(n)]
    for index, (_, variables) in enumerate(critical):
        for variable in variables:
            incident[variable].append(index)

    white_variable = [False] * n
    white_clause = [False] * c1
    queue = deque()
    for variable, count in enumerate(supported):
        if count == 0:
            white_variable[variable] = True
            queue.append(variable)

    while queue:
        variable = queue.popleft()
        for clause_index in incident[variable]:
            if white_clause[clause_index]:
                continue
            white_clause[clause_index] = True
            supporter = critical[clause_index][0]
            supported[supporter] -= 1
            if supported[supporter] == 0 and not white_variable[supporter]:
                white_variable[supporter] = True
                queue.append(supporter)

    core = sum(not is_white for is_white in white_variable)
    return c1, v1, core / n


def enumerate_solutions(clauses, n):
    full_mask = (1 << n) - 1
    solutions = []
    for assignment in range(1 << n):
        for positive, negative, _ in clauses:
            if not ((assignment & positive) | ((full_mask ^ assignment) & negative)):
                break
        else:
            c1, v1, core = whitening_statistics(
                assignment, clauses, n, full_mask
            )
            solutions.append((c1, v1, core))
    return solutions


def weighted_row(seed, solutions, b, t):
    weights = [b**v1 * t**c1 for c1, v1, _ in solutions]
    total = sum(weights)
    square_total = sum(weight * weight for weight in weights)

    def average(index):
        return sum(weight * solution[index] for weight, solution in zip(weights, solutions)) / total

    return {
        "seed": seed,
        "solutions": len(solutions),
        "b": b,
        "t": t,
        "mean_c1": average(0),
        "mean_v1": average(1),
        "mean_core_fraction": average(2),
        "ess": total * total / square_total,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--k", type=int, default=4)
    parser.add_argument("--n", type=int, default=20)
    parser.add_argument("--alpha", type=float, default=9.7)
    parser.add_argument("--seeds", default="1,2,3,4,5")
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    seeds = [int(seed) for seed in args.seeds.split(",")]
    args.out.parent.mkdir(parents=True, exist_ok=True)
    fields = [
        "seed",
        "solutions",
        "b",
        "t",
        "mean_c1",
        "mean_v1",
        "mean_core_fraction",
        "ess",
        "seconds",
    ]
    with args.out.open("w", newline="") as stream:
        writer = csv.DictWriter(
            stream, fieldnames=fields, delimiter="\t", lineterminator="\n"
        )
        writer.writeheader()
        for seed in seeds:
            started = time.perf_counter()
            formula = make_formula(args.k, args.n, args.alpha, seed)
            solutions = enumerate_solutions(formula, args.n)
            elapsed = time.perf_counter() - started
            print(
                f"seed={seed} solutions={len(solutions)} seconds={elapsed:.2f}",
                flush=True,
            )
            if not solutions:
                writer.writerow(
                    {
                        "seed": seed,
                        "solutions": 0,
                        "b": 1.0,
                        "t": 1.0,
                        "mean_c1": 0.0,
                        "mean_v1": 0.0,
                        "mean_core_fraction": 0.0,
                        "ess": 0.0,
                        "seconds": elapsed,
                    }
                )
                continue
            for b in B_VALUES:
                for t in T_VALUES:
                    row = weighted_row(seed, solutions, b, t)
                    row["seconds"] = elapsed
                    writer.writerow(row)


if __name__ == "__main__":
    main()
