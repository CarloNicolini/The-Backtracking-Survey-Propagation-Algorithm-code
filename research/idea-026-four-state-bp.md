# Idea 026 — four-state forcing-bit BP

## Mechanism

The test model reweights satisfying assignments by

`weight = b^V1 * t^C1`,

where `C1` counts critical clauses with exactly one satisfying literal and
`V1` counts variables supporting at least one critical clause. The four-state
messages carry `(sigma, w)`, where `w` records whether the edge is uniquely
satisfying. The prototype computes variable-to-clause and clause-to-variable
messages with explicit generating polynomials over forcing bits.

## Correctness gate

A two-clause tree with three variables was enumerated exactly. The BP edge
beliefs agree with exact biased marginals to `1.99e-13` in total variation.
The equations and forcing-bit indexing therefore pass the tree test.

## Loopy accuracy

At K=4, N=16, alpha=9.7, 30 seeds:

- 24 of 30 BP runs converged;
- 21 formulas were satisfiable and 9 had no solutions;
- mean edge-marginal TV error was `0.3398`;
- mean variable-marginal TV error was `0.2172`;
- greedy sign agreement with exact biased marginals was `0.721`.

For satisfiable formulas, edge TV was `0.2711`, variable TV `0.2308`, and
greedy agreement `0.801`. For UNSAT formulas, edge TV was `0.5`, variable TV
`0.1854`, and greedy agreement `0.535`. One formula with few solutions gave
essentially machine-precision agreement, showing that the implementation can
recover a tractable local measure. The dense hard cases remain poorly
approximated and six runs failed to converge.

## Exact biased greedy oracle

An independent exact oracle removed BP error from the policy test. It fully
enumerated 50 random K=4, N=18, alpha=9.7 formulas. Thirty-one formulas were
satisfiable and all policies reached a solution on all 31.

Mean final whitening-core fraction:

- uniform: `0.8405`;
- critical-clause bias `b=1, t=0.8`: `0.8333`;
- supported-variable bias `b=0.8, t=1`: `0.8280`;
- combined `b=0.6, t=0.9`: `0.8315`.

Paired changes relative to uniform:

- critical: `-0.00717`, SE `0.00844`, 2 lower / 2 higher / 27 unchanged;
- supported: `-0.01254`, SE `0.00803`, 3 lower / 0 higher / 28 unchanged;
- combined: `-0.00896`, SE `0.00819`, 2 lower / 1 higher / 28 unchanged.

The supported-variable bias is the most consistent, but the absolute effect
is small and usually zero because the greedy path reaches the same solution.

## Decision

Keep the four-state equations and exact reweighting tests as validated
research infrastructure. Do not use this Python BP as a production decimation
oracle yet. Its tree implementation is correct, but loopy accuracy is too weak
on hard formulas and the current implementation is too slow for repeated
fixed-point work.

The 30-seed N=16 comparison took about 86 minutes. Further exhaustive BP
comparisons are limited to N <= 12 or require a compiled implementation with
cached products before larger runs.

Artifacts:

- `tools/four_state_bp.py`
- `tools/biased_greedy.py`
- `results/idea-026-k4-n16-a9.7-bp.tsv`
- `results/idea-026-k4-n18-a9.7-greedy50.tsv`
*** End Patch
