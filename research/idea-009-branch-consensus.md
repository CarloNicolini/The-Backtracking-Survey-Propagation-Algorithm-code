# Idea 009 — SP fixed-point branch consensus

## Hypothesis

Parisi identifies SP non-convergence with the possible coexistence of multiple
SP fixed points. Instead of treating one warm-started fixed point as the whole
state, construct a second branch from an independent deterministic
initialization of every active clause-to-variable message.

On each scheduled decimation step:

1. keep the parent's converged surveys;
2. fork the graph, randomize active messages, and reconverge SP;
3. retain variables whose preferred assignment direction agrees in both
   branches;
4. rank each agreed direction by its worst retained-cluster fraction across
   the two branches.

For agreed true assignments the robust score is

`min(1 - sF_parent, 1 - sF_replica)`;

for agreed false assignments it is

`min(1 - sT_parent, 1 - sT_replica)`.

This is a minimax decision over two landscape branches, not a scalar blend of
surveys. The legacy BSP move schedule remains unchanged for the first test, so
any effect comes from representing fixed-point multiplicity.

## Predictions

- Branch distance should grow before runs that later lose convergence.
- Consensus ranking should avoid moves whose direction depends on the SP
  basin, reducing non-convergence and large fixed-depth drops.
- If the SP fixed point is unique, the replica should converge to the same
  branch and reproduce certainty ranking, making the method harmless apart
  from its constant-factor cost.

The option is `--branch-consensus`. Kill the idea if independently initialized
replicas rarely converge, if branch distance is negligible everywhere, or if
consensus does not improve convergence/smoothness over certainty and
polarization on matched seeds.
