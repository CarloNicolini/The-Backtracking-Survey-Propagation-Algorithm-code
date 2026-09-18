# Lyapunov findings

## The tangent calculation is valid

The analytic matrix-free Jacobian-vector product agrees with central finite
differences to relative error `6.89e-11` at the initial fixed point and
`8.03e-11` after `ptrV` has been reordered by decimation/backtracking.
Lyapunov diagnostics leave the parent Sigma trajectory and outcome unchanged.

## Negative complexity and dynamical stability are distinct

For an undecomposed random 3-SAT instance with `N=10000`, `alpha=4.3`,
the production and tightly refined states give:

- `Sigma = -63.1698`;
- `rho(J) = 0.98205`;
- `lambda_max = -0.01811`.

Thus SP has an attractive message fixed point beyond the SAT threshold while
the 1RSB complexity is negative. This directly confirms that iteration
stability, thermodynamic complexity, and satisfiability are separate labels.

## Fatal BSP moves approach a spinodal

At the last converged checkpoints of four failing `N=300`, `alpha=4.15`
certainty runs, the production-state growth estimates were
`{0.998995, 0.990405, 1.011458, 1.002793}`. None could be refined to a tight
fixed point even with 8,192 sweeps.

Earlier checkpoints on the same runs refined successfully with
`rho={0.96271, 0.96830, 0.96315, 0.98673}`. A successful control run at a
comparable late checkpoint had `rho=0.88368`. The disappearance of the tight
attractor therefore precedes the fatal move and is not equivalent to low
Sigma alone.

## Certainty does not maximize stability

At seed 1, checkpoint 1400, all variable/direction pairs were tested:

- certainty also maximized post-move Sigma: drop `0.02393`, `rho=0.97214`;
- polarization: drop `0.02451`, `rho=0.96905`;
- the most stable move: drop `0.19315`, `rho=0.93245`.

There is a real Pareto tradeoff: the most dynamically stable move can destroy
far more clusters.

At the fatal checkpoint 1836:

- the actual certainty pair had no tight fixed point;
- 40 of 380 direction pairs remained feasible;
- the second pair in assignment-retention order was the best-Sigma feasible
  move: drop `0.03160`, `rho=0.99448`;
- the most stable pair had drop `0.11651`, `rho=0.94242`.

This motivates stability as a hard feasibility constraint: order pairs by
cluster retention and take the first with a tightly converged `rho<1` state.
The exhaustive implementation in idea 017 confirms the mechanism but is not
computationally viable as an always-on solver.

Data:

- `results/idea-016-lyapunov-findings/findings.tsv`
- `results/idea-016-lyapunov-findings/candidate_summary.tsv`
- full direction trials under `results/idea-016-candidates-seed1-*`
