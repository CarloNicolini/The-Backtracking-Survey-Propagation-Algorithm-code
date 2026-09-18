# Idea 022 — online SP flexibility observables

## Hypothesis

Before implementing the full maximally-flexible-solution formalism, test
whether observables already defined by SP carry an independent early-warning
signal. At every fixed point record over free variables:

- mean `sI`, the fraction of clusters in which a variable is indeterminate;
- mean absolute polarization `|sT-sF|`;
- mean entropy of the three-component survey;
- the change in mean `sI` since the previous fixed point.

Mean `sI` is only an SP indeterminacy proxy; it is not claimed to equal the
flexibility order parameter of Zhao and Zhou. A useful proxy should change
before the complexity collapse or SP death and add information beyond Sigma
and the Lyapunov stability diagnostics.

The option `--flexibility-diag` is read-only. The first experiment compares
fatal and successful N=300 trajectories and measures how well each observable
predicts the next complexity drop and terminal non-convergence.

No move policy will be implemented unless the proxy has predictive value on
held-out seeds.

## Results

At matched fixed depths 25 and 50, mean `sI` separates the observed SAT and
failed runs with AUC 1.0 both on development seeds 1–5 and held-out seeds
6–15. At the same depths, Sigma gives AUC 0.89–0.91. Successful trajectories
have mean `sI` higher by roughly 0.06–0.12 before the terminal paramagnetic
jump.

The candidate oracle shows that one-step flexibility change is not an
independent greedy objective: its correlation with one-step DeltaSigma is
0.88 at a normal checkpoint and 0.80 at the fatal checkpoint. Maximizing
post-fix mean `sI` incurs much larger complexity drops; at the fatal checkpoint
that move also lacks a tightly converged SP fixed point.

Decision: keep mean `sI` as a global state/early-outcome observable, but reject
direct flexibility-maximizing decimation. The full Zhao-Zhou flexibility
measure may still add information beyond this SP proxy and requires its own
derivation.

Artifacts:

- `results/idea-022-k3-n300-a4.15/`
- `results/idea-022-candidates-normal/`
- `results/idea-022-candidates-fatal/`
