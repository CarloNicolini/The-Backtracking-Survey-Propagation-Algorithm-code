#!/usr/bin/env python3
"""Four-state forcing-bit BP validated against exact biased enumeration."""

import argparse
import csv
import time
from pathlib import Path

from biased_enumeration import make_formula, whitening_statistics

J0, J1, V0 = 0, 1, 2


def formula_edges(clauses):
    edge_values = {}
    variable_edges = {}
    clause_edges = [[] for _ in clauses]
    for clause_index, (positive, _, variables) in enumerate(clauses):
        for variable in variables:
            value = 1 if positive & (1 << variable) else -1
            edge = (variable, clause_index)
            edge_values[edge] = value
            variable_edges.setdefault(variable, []).append(edge)
            clause_edges[clause_index].append(edge)
    return edge_values, variable_edges, clause_edges


def normalize(message):
    total = sum(message)
    return [value / total for value in message] if total > 0.0 else [1.0 / 3.0] * 3


def polynomial(factors):
    coefficients = [1.0]
    for zero_mass, one_mass in factors:
        updated = [0.0] * (len(coefficients) + 1)
        for power, coefficient in enumerate(coefficients):
            updated[power] += coefficient * zero_mass
            updated[power + 1] += coefficient * one_mass
        coefficients = updated
    return coefficients


def biased_mass(coefficients, b, t):
    total = 0.0
    for power, coefficient in enumerate(coefficients):
        total += coefficient * (1.0 if power == 0 else b * t**power)
    return total


def forced_mass(coefficients, b, t):
    return sum(coefficient * b * t ** (power + 1) for power, coefficient in enumerate(coefficients))


def variable_message(edge, clause_messages, variable_edges, edge_values, b, t):
    variable, clause_index = edge
    satisfying_value = edge_values[edge]
    output = [0.0, 0.0, 0.0]
    for state in (J0, J1, V0):
        state_value = satisfying_value if state in (J0, J1) else -satisfying_value
        factors = []
        for other_edge in variable_edges[variable]:
            if other_edge[1] == clause_index:
                continue
            message = clause_messages[other_edge]
            if state_value == edge_values[other_edge]:
                factors.append((message[J0], message[J1]))
            else:
                factors.append((message[V0], 0.0))
        coefficients = polynomial(factors)
        if state == J0:
            output[state] = biased_mass(coefficients, b, t)
        elif state == J1:
            output[state] = forced_mass(coefficients, b, t)
        else:
            output[state] = biased_mass(coefficients, b, t)
    return normalize(output)


def clause_message(clause_index, target_edge, variable_messages, clause_edges):
    terms = [variable_messages[edge] for edge in clause_edges[clause_index] if edge != target_edge]
    u0 = 1.0
    t0 = 1.0
    for message in terms:
        u0 *= message[V0]
        t0 *= message[J0] + message[V0]
    e0 = 0.0
    e1 = 0.0
    for index, message in enumerate(terms):
        product_v = 1.0
        for other_index, other in enumerate(terms):
            if other_index != index:
                product_v *= other[V0]
        e0 += message[J0] * product_v
        e1 += message[J1] * product_v
    return normalize([t0 - u0, u0, max(0.0, t0 - u0 - e0 + e1)])


def run_bp(clauses, edge_values, variable_edges, clause_edges, b, t, damping, tolerance, max_iterations):
    variable_messages = {edge: [1.0 / 3.0] * 3 for edges in variable_edges.values() for edge in edges}
    clause_messages = {edge: [1.0 / 3.0] * 3 for edges in clause_edges for edge in edges}
    for iteration in range(max_iterations):
        change = 0.0
        next_variable = {}
        for variable, edges in variable_edges.items():
            for edge in edges:
                proposal = variable_message(edge, clause_messages, variable_edges, edge_values, b, t)
                previous = variable_messages[edge]
                current = [damping * old + (1.0 - damping) * new for old, new in zip(previous, proposal)]
                next_variable[edge] = current
                change = max(change, max(abs(old - new) for old, new in zip(previous, current)))
        variable_messages = next_variable
        next_clause = {}
        for clause_index, edges in enumerate(clause_edges):
            for edge in edges:
                proposal = clause_message(clause_index, edge, variable_messages, clause_edges)
                previous = clause_messages[edge]
                current = [damping * old + (1.0 - damping) * new for old, new in zip(previous, proposal)]
                next_clause[edge] = current
                change = max(change, max(abs(old - new) for old, new in zip(previous, current)))
        clause_messages = next_clause
        if change < tolerance:
            return variable_messages, clause_messages, iteration + 1
    return variable_messages, clause_messages, max_iterations


def exact_edge_marginals(clauses, n, b, t):
    full_mask = (1 << n) - 1
    counts = {}
    total_weight = 0.0
    solution_count = 0
    for assignment in range(1 << n):
        if not all((assignment & positive) | ((full_mask ^ assignment) & negative)
                   for positive, negative, _ in clauses):
            continue
        solution_count += 1
        c1, v1, _ = whitening_statistics(assignment, clauses, n, full_mask)
        weight = b**v1 * t**c1
        total_weight += weight
        satisfying_count = [
            bin((assignment & positive) | ((full_mask ^ assignment) & negative)).count("1")
            for positive, negative, _ in clauses
        ]
        for clause_index, (positive, _, variables) in enumerate(clauses):
            for variable in variables:
                value = 1 if assignment & (1 << variable) else -1
                satisfying_value = 1 if positive & (1 << variable) else -1
                forcing = value == satisfying_value and satisfying_count[clause_index] == 1
                state = J1 if forcing else (J0 if value == satisfying_value else V0)
                counts[(variable, clause_index, state)] = counts.get((variable, clause_index, state), 0.0) + weight
    return {key: value / total_weight for key, value in counts.items()}, total_weight, solution_count


def bp_variable_marginals(variable_messages, clause_messages, variable_edges, edge_values, b, t):
    marginals = {}
    for variable, edges in variable_edges.items():
        values = [0.0, 0.0]
        for sign in (1, -1):
            factors = []
            for edge in edges:
                message = clause_messages[edge]
                if sign == edge_values[edge]:
                    factors.append((message[J0], message[J1]))
                else:
                    factors.append((message[V0], 0.0))
            coefficients = polynomial(factors)
            values[0 if sign == 1 else 1] = biased_mass(coefficients, b, t)
        total = sum(values)
        marginals[variable] = [value / total for value in values]
    return marginals


def exact_variable_marginals(clauses, n, b, t):
    full_mask = (1 << n) - 1
    values = [0.0] * (2 * n)
    total_weight = 0.0
    for assignment in range(1 << n):
        if not all((assignment & positive) | ((full_mask ^ assignment) & negative)
                   for positive, negative, _ in clauses):
            continue
        c1, v1, _ = whitening_statistics(assignment, clauses, n, full_mask)
        weight = b**v1 * t**c1
        total_weight += weight
        for variable in range(n):
            values[variable * 2 + (0 if assignment & (1 << variable) else 1)] += weight
    if total_weight == 0.0:
        return {variable: [0.5, 0.5] for variable in range(n)}, 0.0
    return {
        variable: [values[variable * 2] / total_weight, values[variable * 2 + 1] / total_weight]
        for variable in range(n)
    }, total_weight


def total_variation(left, right):
    return 0.5 * sum(abs(a - b) for a, b in zip(left, right))


def self_test():
    clauses = [(1 << 0, 1 << 1, [0, 1]), (1 << 1, 1 << 2, [1, 2])]
    edge_values, variable_edges, clause_edges = formula_edges(clauses)
    variable_messages, clause_messages, iterations = run_bp(
        clauses, edge_values, variable_edges, clause_edges, 0.8, 0.9, 0.2, 1e-12, 5000
    )
    exact, _, _ = exact_edge_marginals(clauses, 3, 0.8, 0.9)
    worst = 0.0
    for edge in variable_messages:
        belief = normalize([
            variable_messages[edge][state] * clause_messages[edge][state]
            for state in range(3)
        ])
        expected = [exact.get((edge[0], edge[1], state), 0.0) for state in range(3)]
        worst = max(worst, total_variation(belief, expected))
    print(f"tree_self_test iterations={iterations} max_edge_tv={worst:.3e}")
    return worst < 1e-8


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--k", type=int, default=4)
    parser.add_argument("--n", type=int, default=20)
    parser.add_argument("--alpha", type=float, default=9.7)
    parser.add_argument("--b", type=float, default=0.8)
    parser.add_argument("--t", type=float, default=0.9)
    parser.add_argument("--damping", type=float, default=0.5)
    parser.add_argument("--tolerance", type=float, default=1e-10)
    parser.add_argument("--iterations", type=int, default=2000)
    parser.add_argument("--seeds", default="1,2,3,4,5")
    parser.add_argument("--out", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        raise SystemExit(0 if self_test() else 1)
    if args.out is None:
        parser.error("--out is required")

    args.out.parent.mkdir(parents=True, exist_ok=True)
    fields = [
        "seed",
        "solutions",
        "weighted_mass",
        "iterations",
        "converged",
        "mean_edge_tv",
        "max_edge_tv",
        "mean_var_tv",
        "max_var_tv",
        "greedy_agreement",
    ]
    with args.out.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        for seed_text in args.seeds.split(","):
            seed = int(seed_text)
            started = time.perf_counter()
            clauses = make_formula(args.k, args.n, args.alpha, seed)
            edge_values, variable_edges, clause_edges = formula_edges(clauses)
            exact, total_weight, solution_count = exact_edge_marginals(
                clauses, args.n, args.b, args.t
            )
            variable_messages, clause_messages, iterations = run_bp(
                clauses, edge_values, variable_edges, clause_edges,
                args.b, args.t, args.damping, args.tolerance, args.iterations
            )
            errors = []
            for edge, message in variable_messages.items():
                belief = normalize([
                    message[state] * clause_messages[edge][state]
                    for state in range(3)
                ])
                expected = [exact.get((edge[0], edge[1], state), 0.0) for state in range(3)]
                errors.append(total_variation(belief, expected))
            exact_vars, _ = exact_variable_marginals(clauses, args.n, args.b, args.t)
            bp_vars = bp_variable_marginals(
                variable_messages, clause_messages, variable_edges, edge_values,
                args.b, args.t
            )
            var_errors = [
                total_variation(bp_vars[variable], exact_vars[variable])
                for variable in range(args.n)
            ]
            greedy_agreement = sum(
                (bp_vars[variable][0] > bp_vars[variable][1])
                == (exact_vars[variable][0] > exact_vars[variable][1])
                for variable in range(args.n)
            )
            converged = iterations < args.iterations
            writer.writerow({
                "seed": seed,
                "solutions": solution_count,
                "weighted_mass": total_weight,
                "iterations": iterations,
                "converged": int(converged),
                "mean_edge_tv": sum(errors) / len(errors),
                "max_edge_tv": max(errors),
                "mean_var_tv": sum(var_errors) / len(var_errors),
                "max_var_tv": max(var_errors),
                "greedy_agreement": greedy_agreement,
            })
            print(
                f"seed={seed} solutions={solution_count} iterations={iterations} "
                f"converged={converged} edge_tv={sum(errors) / len(errors):.6g} "
                f"var_tv={sum(var_errors) / len(var_errors):.6g} "
                f"greedy={greedy_agreement}/{args.n} "
                f"seconds={time.perf_counter() - started:.2f}",
                flush=True,
            )


if __name__ == "__main__":
    main()
