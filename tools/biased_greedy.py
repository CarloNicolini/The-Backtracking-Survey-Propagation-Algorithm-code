#!/usr/bin/env python3
"""Compare greedy decimation under uniform and entropy-biased measures."""

import argparse
import csv
import math
import time
from pathlib import Path

from biased_enumeration import make_formula, whitening_statistics


def enumerate_solutions(clauses, n):
    full_mask = (1 << n) - 1
    solutions = []
    for assignment in range(1 << n):
        if all((assignment & positive) | ((full_mask ^ assignment) & negative)
               for positive, negative, _ in clauses):
            c1, v1, core = whitening_statistics(assignment, clauses, n, full_mask)
            solutions.append((assignment, c1, v1, core))
    return solutions


def assignment_weight(features, b, t):
    _, c1, v1, _ = features
    return b**v1 * t**c1


def greedy_decimate(solutions, n, b, t):
    active = list(range(len(solutions)))
    remaining = set(range(n))
    assigned = {}
    while remaining:
        best_variable = None
        best_mass = -1.0
        best_direction = 0
        for variable in sorted(remaining):
            masses = [0.0, 0.0]
            for index in active:
                assignment = solutions[index][0]
                sign = 0 if assignment & (1 << variable) else 1
                masses[sign] += assignment_weight(solutions[index], b, t)
            confidence = max(masses)
            if confidence > best_mass:
                best_mass = confidence
                best_variable = variable
                best_direction = 0 if masses[0] >= masses[1] else 1
        assigned[best_variable] = best_direction
        remaining.remove(best_variable)
        active = [
            index for index in active
            if ((solutions[index][0] >> best_variable) & 1) == (1 - best_direction)
        ]
        if not active:
            return None
    return solutions[active[0]]


def summarize(rows):
    count = len(rows)
    return {
        "solved": count,
        "mean_core": sum(row[3] for row in rows) / count,
        "mean_c1": sum(row[1] for row in rows) / count,
        "mean_v1": sum(row[2] for row in rows) / count,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--k", type=int, default=4)
    parser.add_argument("--n", type=int, default=16)
    parser.add_argument("--alpha", type=float, default=9.7)
    parser.add_argument("--seeds", default="1,2,3,4,5,6,7,8,9,10")
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    args.out.parent.mkdir(parents=True, exist_ok=True)
    fields = [
        "seed",
        "solutions",
        "policy",
        "b",
        "t",
        "solved",
        "core_fraction",
        "c1",
        "v1",
        "seconds",
    ]
    policies = [
        ("uniform", 1.0, 1.0),
        ("critical", 1.0, 0.8),
        ("supported", 0.8, 1.0),
        ("combined", 0.6, 0.9),
    ]
    with args.out.open("w", newline="") as stream:
        writer = csv.DictWriter(
            stream, fieldnames=fields, delimiter="\t", lineterminator="\n"
        )
        writer.writeheader()
        for seed_text in args.seeds.split(","):
            seed = int(seed_text)
            started = time.perf_counter()
            formula = make_formula(args.k, args.n, args.alpha, seed)
            solutions = enumerate_solutions(formula, args.n)
            for policy, b, t in policies:
                result = greedy_decimate(solutions, args.n, b, t)
                solved = result is not None
                row = {
                    "seed": seed,
                    "solutions": len(solutions),
                    "policy": policy,
                    "b": b,
                    "t": t,
                    "solved": int(solved),
                    "core_fraction": result[3] if solved else "",
                    "c1": result[1] if solved else "",
                    "v1": result[2] if solved else "",
                    "seconds": time.perf_counter() - started,
                }
                writer.writerow(row)
            print(
                f"seed={seed} solutions={len(solutions)} "
                f"seconds={time.perf_counter() - started:.2f}",
                flush=True,
            )


if __name__ == "__main__":
    main()
