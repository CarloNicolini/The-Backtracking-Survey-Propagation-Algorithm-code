# Idea 014 — adiabatic two-stage Parisi exchange

## Hypothesis

The current `I(k)` estimator accurately predicts release gain, but applying
release and replacement simultaneously destroys that gain through nonlinear
SP re-equilibration. Make Parisi's equation (5) adiabatic:

1. in a normal state, compare `P_max` with current `I_min`;
2. when `P_max > I_min`, release only `I_min`;
3. reconverge SP on the enlarged residual problem;
4. choose and fix the best replacement from the updated cavity state, excluding
   the just-released variable;
5. reconverge, then force one ordinary decimation to guarantee net progress.

If `P_max <= I_min`, decimate directly. This is a state machine with no
backtracking ratio or score parameter. It preserves the accurately measured
release response and lets the replacement adapt to that response before the
fixed cardinality is restored.

## Predictions

- Release transitions should raise Sigma by approximately `-log(I_min)`.
- Replacement should lose less of that credit than the simultaneous exchange
  used in idea 005.
- The fixed-depth Sigma frontier should exceed certainty while retaining
  deterministic progress every three transitions at worst.
- SP non-convergence should decrease if simultaneous nonlinear shocks caused
  the earlier failure.

The option is `--two-stage-exchange`. Kill it if re-equilibration does not
recover the release credit, if it cycles despite forced progress, or if
success/frontier smoothness regress on multiple seeds.
