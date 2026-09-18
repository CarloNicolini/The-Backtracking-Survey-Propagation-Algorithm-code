# Idea 018 — tight-and-physical BSP

## Hypothesis

The exhaustive Lyapunov oracle is correct but too expensive. A proposal that
actually converges under the exact SP sweep to `ZERO` tolerance has already
demonstrated attraction along its basin; calculating 128 additional tangent
sweeps for every candidate is unnecessary as a first feasibility filter.

Order variable/direction pairs by assignment-specific cluster retention and
accept the first proposal satisfying both:

1. SP converges to `ZERO` within the extended diagnostic budget;
2. post-move complexity is nonnegative.

Apply the same test to scheduled releases in the legacy ordering. This jointly
enforces dynamical existence and physical cluster feasibility. It has no
Lyapunov margin, complexity buffer, shortlist size, or weighted score.

## Predictions

- Normal moves reproduce certainty because the first pair is feasible.
- The fatal seed-1 certainty move is rejected and the second retention pair is
  accepted.
- The controller cannot follow the attractive but negative-complexity cycle
  observed in idea 017.
- Runtime should fall sharply because candidate power iterations are removed.

The option is `--tight-physical`. Promote only if it preserves or improves
success and complexity smoothness while reducing the idea-017 cost by orders
of magnitude.

## Pilot result

On N=300 seed 1, the filter avoids the original failure at step 1836 and
continues to step 2049 while keeping Sigma nonnegative. It still terminates
without a feasible move. Runtime is 239 s versus 0.54 s for certainty: lower
than the full Lyapunov oracle, but far outside the algorithmic budget.

Decision: kill the per-candidate tight filter. The expensive operation is
refining each proposed state, not the tangent power iteration alone. A
practical continuation must carry an online Lyapunov direction between BSP
steps and update it with O(1) Jacobian-vector products.
