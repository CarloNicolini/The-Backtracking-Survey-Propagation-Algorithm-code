# Idea 010 — transaction-safe legacy BSP

## Hypothesis

Keep the legacy `r=0.9` decimation/backtracking schedule and its complexity
trajectory, but make every scheduled decimation transactional. Probe the
highest-scored variable in a forked graph. If SP reconverges, commit exactly
the legacy move. If it would kill SP:

1. probe the opposite assignment direction;
2. walk down the existing score ordering until the first convergent fix;
3. if every forward move fails, probe and release the minimum-current-I
   fixation.

Failed proposals cannot corrupt the parent because they terminate inside
copy-on-write children. Accepted proposals are replayed in the parent and
their Sigma must match exactly.

This is a safety layer around the existing dynamics, not a replacement
schedule or score. Therefore ordinary trajectories should preserve certainty's
move-depth and fixed-depth smoothness, while the first fatal move becomes a
search event rather than terminal non-convergence.

The first implementation commits one variable per scheduled decimation so the
N=300 and N=1000 experiments are directly controlled; target-scale batching
will be generalized only if this mechanism wins.

## Predictions

- Before the first fatal proposal, curves and selected moves should match
  certainty exactly.
- Some known non-convergent seeds should reach SAT through an alternative
  direction or variable.
- Replay error must remain zero.
- Normal-path wall time should be roughly two SP solves per decimation step;
  expensive scans should occur only near a baseline death.

The option is `--transaction-safe`. Kill it if no failures are rescued, if
accepted probes diverge on replay, or if smoothness changes substantially
before a recovery event.
