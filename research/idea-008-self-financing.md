# Idea 008 — self-financing compound progress

## Hypothesis

Expand the solver's action space rather than tuning a variable score. From
each converged SP state, construct two proposals with positive net progress:

1. direct: fix the free variable with maximum `P(i)`;
2. compound: release the fixed variable with minimum current `I(k)`, then fix
   the best two mutually clause-separated free variables.

The direct move changes fixed cardinality by `+1`; the compound changes it by
`-1 + 2 = +1`. Probe both in fork-isolated graph copies, fully reconverge SP,
and choose the lower measured complexity loss per actual net fixation:

`cost = (Sigma_before - Sigma_after) / (q_after - q_before)`.

The release in the compound move supplies a measured complexity credit that
can pay for an otherwise costly fixation. Because both candidates make equal
progress, comparing their residual Sigma directly optimizes the physical
frontier `Sigma(q)`. No backtracking ratio, score exponent, threshold, or
shortlist size is selected.

If both proposals fail to converge, reuse the certified scheduler's recovery
ladder. Accepted moves are replayed in the parent and must match child Sigma
to `1e-8`.

## Predictions

- Compound moves should be concentrated near impending large direct drops.
- The fixed-depth frontier should have lower maximum drop and curvature than
  certainty and polarization while preserving or improving SAT outcomes.
- Runtime should remain below three times certainty: two probes plus one
  committed SP solve per net-progress step.
- If the compound action is useful, its measured cost should beat the direct
  proposal on a nontrivial fraction of steps without causing replay failures.

Kill the idea if compounds are almost never selected, if they worsen
fixed-depth smoothness, or if nonlinear multi-variable interactions erase the
release credit despite fork certification.
