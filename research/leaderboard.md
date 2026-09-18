# Complexity-policy leaderboard

Metrics are computed between consecutive SP fixed points. `mean_abs_delta`
measures roughness, `mean_drop` is `(Sigma_initial - Sigma_final) / steps`,
and `auc_per_step` is the trapezoidal area under Sigma divided by the number
of transitions. Lower drop/roughness and higher area are preferred, provided
that success and convergence do not regress.

## Idea 001 — exact one-step Sigma lookahead: killed

Hypothesis: from the top four variables under the current scorer, fork all
eight `(variable, direction)` trials, reconverge SP, and fix the converged pair
with maximum residual Sigma. Trials run concurrently and the option is exposed
as `--lookahead-k=4`.

Pilot: random 3-SAT, `N=300`, `alpha=4.15`, `r=0.9`, seeds 1–3. Certainty,
polarization, and lookahead used the same formulas and fixed one variable per
SP convergence. All nine runs ended in SP non-convergence.

- Certainty: mean max drop 0.0301853; mean absolute delta 0.0161131; mean
  area/step 1.43474; mean 1,572 steps; mean 0.437 s.
- Polarization: mean max drop 0.0563492; mean absolute delta 0.0196167; mean
  area/step 1.47982; mean 1,116 steps; mean 0.436 s.
- Lookahead-4: mean max drop 0.0292939; mean absolute delta 0.0163055; mean
  area/step 1.41870; mean 1,602 steps; mean 51.832 s.

Decision: kill. Relative to certainty, lookahead reduced the maximum drop by
only 2.95%, increased mean roughness by 1.19%, reduced area/step by 1.12%, did
not prevent any non-convergence, and cost 118.7 times more wall-clock time.
The tiny `N=80`, seed-1 smoke test solved under all policies and is treated
only as a correctness check.

Artifacts:

- `results/idea-001-pilot-k3-n300-a4.15/summary.tsv`
- `results/idea-001-pilot-k3-n300-a4.15/aggregate.tsv`
- `results/idea-001-pilot-k3-n300-a4.15/sigma_curves.svg`
- Per-run full curves in each `trace_steps.csv` below that directory.

Implementation commit: `1a933cf`. Results commit: `112dfc9`.

## Idea 002 — relative direction-margin gate: killed

Hypothesis: retain certainty's low immediate complexity cost, but skip
variables whose assignment direction has normalized margin
`|sT-sF|/(sT+sF) < theta`. This uses the existing `--theta` path and adds no
message-passing work.

Pilot: random 3-SAT, `N=300`, `r=0.9`, seeds 1–5, at `alpha=4.0` and `4.15`;
`theta` in `{0.25, 0.5, 0.75}`. At `alpha=4.0`, `theta=0.5` changed certainty
from 2/5 SAT, 2/5 SP non-convergence, and 1/5 WalkSAT failure to 3/5 SAT and
2/5 SP non-convergence. Its mean max drop (1.59031 versus 1.58153), roughness
(0.0332335 versus 0.0332072), and area/step (3.86240 versus 3.84691) were
otherwise effectively unchanged. At `alpha=4.15`, every threshold produced
curves and outcomes identical to certainty: 1/5 SAT and 4/5 SP
non-convergence.

Decision: kill the relative-margin gate as a primary policy. It repaired one
low-alpha WalkSAT outcome at negligible SP cost, but did not flatten Sigma and
became completely inactive closer to the hard regime. Relative margin can be
large when both `sT` and `sF` are tiny, so it does not reliably detect the
high-`sI`, low-information moves it was intended to veto. The next test should
regularize certainty with absolute polarization rather than a ratio.

Artifacts:

- `results/idea-002-pilot-k3-n300-a4.0/`
- `results/idea-002-pilot-k3-n300-a4.15/`
- Full certainty, polarization, and `theta=0.5` curves are committed; each
  directory also contains all-policy summaries and an SVG comparison.

Hypothesis commit: `6bfaa01`. Results commit: `94fe768`.
