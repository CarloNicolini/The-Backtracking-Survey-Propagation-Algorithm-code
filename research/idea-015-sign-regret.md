# Idea 015 — sign-regret mistake correction

## Hypothesis

Minimum current `I(k)` identifies the easiest assignment to release, but an
easy release can be harmless. Target actual mistakes instead. Reconstruct the
two current assignment-retention factors for every fixed variable:

- true retention: `P_T = B/Z`;
- false retention: `P_F = A/Z`.

For the value currently assigned to k, define

`regret(k) = log(max(P_T, P_F) / P_assigned)`.

Regret is exactly zero while the fixed sign remains locally preferred and
positive only after the preferred sign flips.

During a scheduled BSP backtracking slot, release the largest-regret
non-forced variables. If no fixation has positive regret, replace that
unnecessary backtrack with an ordinary decimation. Thus the nominal schedule
is only an opportunity to correct detected sign mistakes; it no longer
releases variables merely because they are weakly constrained.

The policy has no score exponent, threshold, or new backtracking ratio and is
exposed as `--sign-regret-backtrack`.

## Predictions

- Released variables should be enriched for assignments that would otherwise
  precede large Sigma drops or SP failure.
- Avoiding zero-regret releases should reduce churn and wall time while
  preserving a higher fixed-depth Sigma frontier.
- Success should improve over minimum-I backtracking because releases encode
  changed directional evidence, not only local freedom.

Kill the idea if sign flips are too rare to repair failures, if progress
becomes too aggressive, or if success/frontier smoothness regress.
