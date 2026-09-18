# Idea 013 — failure-triggered SP basin jump

## Hypothesis

Parisi suggests that SP non-convergence can reflect multiple fixed points.
Idea 009 paid for an alternate branch at every decimation and failed. Here the
alternate branch exists only as a hard-event recovery:

1. transactionally probe the scheduled BSP fix or release from its warm start;
2. if that probe fails, keep the identical graph mutation but independently
   reinitialize every active clause-to-variable message;
3. accept the move if this second SP branch converges;
4. replay both the graph mutation and the exact deterministic initialization
   in the parent.

Only after the basin jump fails does the existing direction/variable/current-I
recovery ladder run. This tests fixed-point multiplicity at the state where it
matters, without changing assignments or paying an always-on replica cost.

One independent initialization is part of the algorithm, not a tuned replica
count. Replay Sigma must remain exact.

## Predictions

- Some warm-start failures should converge from the alternate message basin.
- Successful basin jumps should reduce exhaustive alternative scans and rescue
  additional seeds without changing the pre-failure Sigma trajectory.
- Normal-path overhead is unchanged from transaction-safe BSP; each fatal
  proposal adds only one SP attempt.

Kill the idea if alternate initializations never rescue a move, if replay
diverges, or if rescued branches have worse success/smoothness than assignment
recovery alone.
