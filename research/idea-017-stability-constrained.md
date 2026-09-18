# Idea 017 — stability-constrained BSP

## Hypothesis

Cluster retention and dynamical stability are distinct objectives. Treat
stability as a feasibility constraint rather than combining it with Sigma
through a tunable weight.

For a scheduled decimation, enumerate assignment pairs in descending

`P(i, true)=1-sF(i)`, `P(i, false)=1-sT(i)`.

Probe each pair in a fork, refine SP to `ZERO`, estimate the analytic largest
Lyapunov growth `rho`, and accept the first pair with a converged fixed point
and `rho < 1`. Because proposals are ordered by retained-cluster fraction,
this is the maximum-retention move among the dynamically attractive states
encountered.

For a scheduled backtrack, order fixed variables by current Parisi `I(k)` and
accept the first release with a tightly converged attractive fixed point.

No score interpolation, stability margin, candidate count, or fitted
threshold is used. The only boundary is the dynamical definition of local
attraction, `rho < 1`.

## Evidence motivating the policy

At N=300, seed 1, checkpoint 1400, certainty selected the smallest measured
complexity drop but not the most stable move. At the fatal checkpoint 1836,
the actual certainty move had no tight fixed point. The second assignment pair
in retention order converged with `rho=0.99448` and nearly the same complexity
drop. Forty of 380 direction trials remained dynamically feasible.

## Predictions

- Normal states should accept the first proposal and reproduce certainty.
- Near a spinodal, a later high-retention pair should replace the fatal move.
- SP non-convergence should fall without paying the large complexity loss of
choosing the globally smallest rho.
- Probe count should remain close to one except near instability.

The option is `--stability-constrained`. The first implementation uses
single-variable moves and is restricted to moderate-N hypothesis testing.
