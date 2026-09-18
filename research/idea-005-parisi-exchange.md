# Idea 005 — event-driven Parisi exchange dynamics

## Structural hypothesis

The fixed backtracking ratio is solving the wrong control problem. Parisi's
equation (5) supplies a parameter-free decision: let `P_max` be the largest
fraction of clusters retained by the best free-variable assignment, and
`I_min` the smallest fraction retained by keeping a currently fixed variable
at its present value. If `P_max > I_min`, simultaneously release the bad
fixation and make the good fixation. The number of free variables is unchanged
and the predicted complexity change is

`Delta Sigma_pred = log(P_max / I_min) > 0`.

Otherwise decimate `P_max` and make progress.

This changes BSP from a scheduled decimation/backtracking mixture into a
state-feedback optimizer. At most one exchange is allowed between two
decimations, which guarantees progress without a backtracking-ratio parameter.
Pairs sharing a clause are excluded because Parisi's gain argument assumes
weakly correlated moves.

## Dynamic I(k)

The legacy backtracker ranks fixed variables by the stale score stored when
they were fixed. Here every incoming clause-to-variable warning for a fixed
variable is reconstructed in O(K) from the current cavity products of the
other clause members. These warnings give current virtual surveys
`(sT, sF, sI)` for the removed variable. Its retained-cluster fraction is

- `I(k) = 1 - sF(k)` when k is fixed true;
- `I(k) = 1 - sT(k)` when k is fixed false.

Thus an assignment that has become incompatible with the evolving residual
state acquires small `I(k)` and is released.

## Falsifiable predictions

1. Exchange transitions should raise Sigma more often than chance, and the
   sign of `log(P_max/I_min)` should agree with the measured next-step change.
2. Sigma should remain above the certainty and polarization trajectories at
   matched fixed-variable count, reducing positive `Delta Sigma` tails.
3. The event-driven policy should use fewer backtracks than fixed-ratio BSP
   when the current assignment remains consistent, while retaining the ability
   to repair early mistakes.

Kill the idea if the local `I(k)` estimator does not predict release gains, if
exchanges systematically lower Sigma, or if progress stalls despite the
one-exchange invariant.
