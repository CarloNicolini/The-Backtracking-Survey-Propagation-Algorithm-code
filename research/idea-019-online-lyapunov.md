# Idea 019 — transported online Lyapunov mode

## Hypothesis

Full local eigensolves correctly identify SP spinodals but are too expensive
for move selection. Initialize one tangent direction at the start of each SP
reconvergence and update it once alongside every message sweep that SP already
performs. This prevents tangent directions from being mixed across different
factor-graph topologies.

The observed norm gives the finite-time growth

`rho_online(t) = ||J_t v_t||`,

then `v_(t+1)=J_t v_t/rho_online(t)`. Removed edges are zeroed; edges
reactivated by backtracking receive deterministic nonzero components.

This approximately doubles sweep work and has no separate power-iteration
count, perturbation amplitude, or fitted threshold. It uses the sweeps already
spent on the current residual formula as an online power iteration instead of
solving a separate local eigenproblem.

## Tests

1. Verify trajectory identity with the diagnostic disabled/enabled.
2. At sampled checkpoints, compare online growth with the full tightly refined
   local `rho(J)`.
3. Test whether online growth approaches one before known fatal moves and
   separates them from successful trajectories.
4. Only if predictive, use the online mode's per-variable participation
   to rank stability-preserving moves without candidate reconvergence.

The option is `--online-lyapunov`.
