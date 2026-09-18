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
