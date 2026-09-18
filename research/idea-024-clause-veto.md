# Idea 024 — clause-separated decimation blocks

## Hypothesis

At target K=3 size, BSP fixes several variables from one pre-move survey
snapshot. The factorized estimate

`log P(block) ≈ sum_i log P_i`

can fail when two selected variables share an active clause. Use the existing
`--veto` option to prevent direct within-clause interactions inside each
decimation batch, while retaining dynamic-I backtracking.

This is a block-structure test, not a new scalar score. At N=300 and N=1000
the batch width is one and the option is inert, so screen at K=3, N=3000,
alpha=4.2, seeds 1–5. Compare:

- certainty;
- polarization;
- dynamic-I;
- dynamic-I plus clause veto.

Promote veto to N=10000 only if it changes a meaningful fraction of blocks,
does not regress success, improves fixed-depth maximum drop and roughness, and
keeps runtime close to dynamic-I.
