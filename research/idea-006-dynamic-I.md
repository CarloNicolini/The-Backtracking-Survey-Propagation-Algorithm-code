# Idea 006 — current-I backtracking

## Hypothesis

The standard BSP schedule is not the main defect; the release decision is.
`surveys()` recomputes surveys only for unfixed variables, so legacy
backtracking sorts fixed variables by `_sC` values frozen at their assignment
times. Those scores compare different residual formulas and cannot detect a
fixation that has become wrong.

Keep the established `r=0.9` schedule and certainty decimation unchanged.
During each backtracking step, reconstruct current virtual surveys for every
non-forced fixed variable and release the smallest

`I(k) = 1 - sF(k)` if k is fixed true, otherwise `1 - sT(k)`.

The dynamic estimator was validated in idea 005 against 120 full
release/reconvergence trials (`r=0.99684`, MAE `0.00056`). This experiment
therefore tests Parisi's backtracking prescription without changing the move
frequency or adding a tunable score.

## Predictions

1. Backtracking transitions should increase Sigma more than legacy
   stale-score releases.
2. At matched number of fixed variables, the Sigma frontier should be higher
   and smoother, delaying SP non-convergence.
3. Runtime per step should increase only by an O(number of fixed edges) local
   scan, with no additional SP solves.

The policy is exposed as `--dynamic-i-backtrack`. Kill it if fixed-depth Sigma
or convergence does not improve over certainty BSP on multiple seeds.
