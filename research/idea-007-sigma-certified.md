# Idea 007 — Sigma-certified transactional scheduler

## Hypothesis

Treat each graph mutation as a proposal, not an irreversible command. At the
last converged SP state, choose the certainty decimation `(i, dir)` and execute
it in a forked copy. Its local cluster-retention prediction is `log(P_i)`,
where `P_i = 1 - min(sT_i, sF_i)`. The proposal is certified when SP converges
and

`Sigma_after - Sigma_before >= log(P_i)`.

Reaching the trivial `Sigma=0` fixed point is also accepted because complexity
has a terminal discontinuity there.

If the proposal is anomalous or non-convergent, compute the validated current
`I(k)` and probe one nonlocal Parisi swap. Commit it only when SP converges,
fixed cardinality is unchanged, and measured Sigma increases. If forward SP
is fatal and the swap fails, probe the release alone and retreat only when its
measured Sigma does not decrease. If no repair is available, try the opposite
direction and then walk down the certainty ordering until the first convergent
forward proposal. This ladder is activated only after a fatal preferred move;
it has no candidate-count parameter. Failed probes die in children, leaving
the parent at its last converged state.

This is algorithmic meta-optimization: the solver observes the global response
of its own proposed action and changes control flow. It has no candidate-count
or fitted score threshold, and normally costs one extra SP solve per accepted
step.

## Predictions and gates

1. Parent replay must reproduce the certified child Sigma to `1e-8`; any
   larger discrepancy is a correctness failure.
2. A known baseline non-convergence should become a rejected proposal or a
   successful Parisi repair rather than terminating the run.
3. Maximum and high-tail pre-terminal drops should fall without reducing SAT
   rate; target overhead must remain below three times certainty BSP.
4. At matched fixed depth, frontier Sigma should remain above the certainty
   and polarization controls.

Start with remote `N=80` invariant/replay tests, then the same five
`N=300, alpha=4.15` seeds used by prior ideas. Promote only after zero replay
or parent-state failures.
