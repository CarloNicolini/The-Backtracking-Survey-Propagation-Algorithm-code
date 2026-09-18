# Idea 002 — certainty with a direction-margin gate

## Hypothesis

Certainty is a cheap proxy for the immediate complexity cost because
`1 - min(sT, sF) = max(sT, sF) + sI`. Its highest-ranked variables can,
however, be dominated by `sI`, leaving the assignment direction dependent on
the small and noisy difference `sT - sF`. Keep certainty ranking, but skip a
candidate unless

`|sT - sF| / (sT + sF) >= theta`.

This should preserve most of certainty's low immediate complexity loss while
avoiding moves with weak directional evidence. It uses the existing
`--theta=THRESHOLD` flag and requires no extra SP reconvergence.

## Pilot protocol

Run random 3-SAT at `N=300`, `r=0.9`, seeds 1–5, at `alpha=4.0` and `4.15`.
Compare certainty, polarization, and certainty with `theta` in `{0.25, 0.5,
0.75}`. The first-fixed-point margin medians on three calibration seeds were
0.552, 0.597, and 0.565, so this grid spans permissive to selective gates.

Promote only if a threshold improves complexity smoothness or convergence
without a material wall-clock penalty. The `theta=0` certainty run is the
control, and all policies fix one variable per SP fixed point at this size.
