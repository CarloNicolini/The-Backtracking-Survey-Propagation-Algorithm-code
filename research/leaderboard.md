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

## Idea 003 — soft absolute-polarization regularization: keep and promote

Hypothesis: rank with
`certainty * |sT-sF|^gamma`, using a small exponent to reject genuinely tiny
direction signals without accepting polarization's full complexity cost. The
existing flag is `--scorer=gamma:G`.

Pilot: random 3-SAT, `N=300`, `r=0.9`, seeds 1–5, at `alpha=4.0` and `4.15`;
`gamma` in `{0.05, 0.1, 0.25, 0.5}`. Smoothness comparisons exclude only an
intentional final transition to exactly `Sigma=0`; the required full-curve
maximum remains in the result files.

At `alpha=4.0`, `gamma=0.1` improved certainty from 2/5 to 4/5 SAT, leaving
one SP non-convergence and eliminating one WalkSAT failure. Its mean
pre-terminal maximum drop fell 6.05% (1.58153 to 1.48586), pre-terminal
roughness fell 0.89% (0.0332223 to 0.0329277), and area/step rose 8.51%
(3.84691 to 4.17444). Normal-run wall time remained about 0.46 s.

At `alpha=4.15`, both policies achieved 1/5 SAT and 4/5 SP
non-convergence. `gamma=0.1` reduced pre-terminal roughness by 1.89%, but
increased the maximum pre-terminal drop by 3.95%; area/step rose 0.33%.
`gamma=0.05` was smoother there, but did not improve outcomes.

Decision: keep `gamma=0.1` as the first serious contender and promote it to
larger `N` and more seeds. The pilot is promising rather than conclusive: the
success gain occurs at the easier alpha, while the harder point remains
limited by SP convergence.

Artifacts:

- `results/idea-003-pilot-k3-n300-a4.0/`
- `results/idea-003-pilot-k3-n300-a4.15/`
- Full certainty, polarization, and `gamma=0.1` curves are committed; each
  directory also contains all-policy summaries and an SVG comparison.

Hypothesis commit: `4a0edff`. Metric correction: `bef5cd3`. Results commit:
`6216388`.
