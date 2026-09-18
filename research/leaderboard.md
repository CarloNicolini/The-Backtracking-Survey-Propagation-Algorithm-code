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

The first larger test showed that `gamma=0.1` was too strong, so the exponent
was refined. At `N=1000`, `alpha=4.15`, 15 seeds, `gamma=0.01` matched
certainty's 13/15 SAT and 2/15 SP non-convergence. It reduced pre-terminal
roughness by 5.90% on average and on 12/15 paired seeds, increased area/step
by 0.44%, and reduced wall time by 3.97%. Its mean maximum drop was 4.83%
larger. Polarization solved only 11/15.

Target validation: random 3-SAT, `N=10000`, `alpha=4.2`, `r=0.9`, seeds 1–5.
`gamma=0.01` and polarization solved 5/5; certainty solved 4/5. Gamma reduced
pre-terminal roughness by 16.92% versus certainty (on every paired seed) and
16.28% versus polarization, with wall time equal to certainty and 9.65% below
polarization. Its area/step was 1.52% below certainty and 5.98% above
polarization. The unresolved cost is the tail: mean maximum drop was 28.37%
above certainty.

Decision: `--scorer=gamma:0.01` is the current winner because its soft
absolute-polarization factor suppresses variables whose assignment direction
is based on a tiny survey difference, while the small exponent largely
preserves certainty's low-cost ranking. At target scale it gives the smoothest
average Sigma trajectory and removes the observed certainty failure without
extra per-step computation. Keep it behind the existing CLI flag, and next
target the larger worst-drop tail rather than increasing gamma.

Artifacts:

- `results/idea-003-pilot-k3-n300-a4.0/`
- `results/idea-003-pilot-k3-n300-a4.15/`
- `results/idea-003-validation-k3-n1000-a4.15-15seeds/`
- `results/idea-003-target-k3-n10000-a4.2-5seeds/`
- Full certainty, polarization, and `gamma=0.1` curves are committed; each
  pilot directory contains all-policy summaries and an SVG comparison.
  Validation directories contain combined summaries and full SVG curves.

Hypothesis commit: `4a0edff`. Metric correction: `bef5cd3`. Results commit:
`6216388`. Scale-validation results: `1080aa6`.

## Idea 005 — event-driven Parisi exchanges: controller killed, estimator kept

Hypothesis: replace the fixed backtracking schedule by Parisi's equation (5).
At each SP fixed point, estimate the best free-variable retention `P_max` and
the weakest current fixation `I_min`; exchange them at constant fixed depth
when `P_max > I_min`, otherwise decimate. Exchanges were relaxed while
measured Sigma increased, making complexity a Lyapunov feedback signal.

The key new component worked. On one `N=300`, `alpha=4.15` audit, 120 forked
release/reconvergence trials gave Pearson `r=0.99684` between `-log(I_min)` and
the measured complexity gain. All 120 gains had the predicted positive sign;
mean predicted and measured gains were 0.02263 and 0.02232, with MAE 0.00056.
This validates the O(degree*K) reconstruction of current fixed-variable
surveys and removes the legacy dependence on stale fixation-time scores.

The simultaneous exchange controller did not generalize. At `N=300`, five
seeds, it matched certainty's 1/5 SAT, reduced per-transition roughness 6.9%
and worst drop 8.0%, and was 4.5 times faster. At `N=1000`, however, it solved
4/5 versus certainty's 5/5; its fixed-depth frontier was 33.6% rougher and its
frontier area/level 2.2% lower. Applying release and fixation simultaneously
therefore loses the accurately predicted release gain through nonlinear
re-equilibration.

Decision: kill the event-driven simultaneous-exchange controller. Keep the
dynamic `I(k)` estimator and test the more faithful intervention: retain the
standard BSP schedule but replace stale-score backtracking with the current
`I(k)` ordering.

Artifacts:

- `results/idea-005-audit-k3-n300-a4.15/`
- `results/idea-005-relax-k3-n300-a4.15/`
- `results/idea-005-relax-k3-n1000-a4.15/`

Implementation: `d5e6db0`. Release audit: `501e9cb`. Lyapunov refinement:
`71ccef2`. Results: `679ace9`.

## Idea 006 — current-I backtracking under the legacy schedule: killed

Hypothesis: preserve the standard `r=0.9` BSP schedule and change only release
selection from stale fixation-time `_sC` to the validated current `I(k)`.

At `N=300`, five seeds, outcomes were unchanged at 1/5 SAT. Dynamic-I reduced
mean worst drop by 4.8%, but increased per-transition roughness by 4.6%,
reduced fixed-depth frontier area by 1.8%, and added 19.6% wall time.

At `N=1000`, five seeds, dynamic-I solved 4/5 versus certainty's 5/5. It
reduced mean worst drop by 6.7% and fixed-depth frontier roughness by 23.2%,
but increased move-depth roughness by 2.5%, reduced frontier area by 1.5%, and
added 26.7% wall time.

Decision: kill the direct substitution. The estimator identifies releases
that increase Sigma locally, but the fixed `r=0.9` schedule demands too many
releases whether or not a profitable replacement exists. The next controller
should gate release by `P_max > I_min`, reconverge after releasing, then choose
the replacement from the updated state instead of applying both changes
simultaneously.

Artifacts:

- `results/idea-006-k3-n300-a4.15/`
- `results/idea-006-k3-n1000-a4.15/`

Implementation: `b064217`. Results: `871f12b`.

## Idea 007 — Sigma-certified transaction scheduler: recovery kept, control killed

Hypothesis: probe the preferred decimation in a fork and certify its measured
response against `log(P)`. An anomalous/fatal proposal receives one measured
Parisi swap; fatal moves then try release, opposite direction, and remaining
certainty candidates. Only converged proposals are replayed in the parent.

The transaction mechanism passed its safety gate: every accepted child move
replayed with exactly zero Sigma error, while failed SP runs remained
child-local. At `N=300`, five seeds, it rescued two formulas and raised SAT
from 1/5 to 3/5. No parent run died by SP non-convergence; two exhausted all
certified alternatives and stopped explicitly. Exhaustive failures raised
wall time about elevenfold, mean pre-terminal worst drop by 9.9%, and
move-depth roughness by 24.5%.

At `N=1000`, it preserved certainty's 5/5 SAT, reduced worst drop by 3.5%,
and cut wall time by 55%. However, rapid default decimation increased
move-depth roughness by 54% and fixed-depth frontier roughness by 33.5%;
frontier area fell 1.4%.

Decision: keep fork isolation, exact replay, and failure recovery—they are the
first components to rescue known non-convergent trajectories. Kill the
always-progress certification law as the primary optimizer. The next
controller must explicitly finance costly net fixation with measured release
credits, rather than treating low move count as success.

Artifacts:

- `results/idea-007-smoke-compare-k3-n80-a4.0/`
- `results/idea-007-recovery-k3-n300-a4.15/`
- `results/idea-007-recovery-k3-n1000-a4.15/`

Implementation: `c8ff796`. Recovery ladder: `1edf603`. Results: `af6f313`.

## Idea 008 — self-financing compound progress: killed

Hypothesis: compare two fork-measured proposals with equal net progress:
directly fix the best free variable, or release the minimum-current-I fixation
and fix two clause-separated variables. Choose the lower measured complexity
loss per actual net fixation.

The mechanism was active and safe: the N=300 runs selected 125 compound moves
with zero replay error. It retained idea 007's recovery gain, solving 3/5
versus 1/5 for both baselines. However, its mean pre-terminal worst drop was
0.4475 versus certainty's 0.4573, while move-depth roughness rose 37.4%.
More importantly, fixed-depth frontier roughness rose 34.2% and frontier
area/level fell 28.1%. Exhaustive recovery on the two unsolved seeds made
mean wall time roughly ten times certainty.

Decision: kill. Expanding the move to `release one + fix two` and measuring it
exactly still gives a myopic greedy policy; a locally superior `q -> q+1`
state can lead to a worse future frontier. Do not tune the compound size. The
next structural test should use SP fixed-point multiplicity itself as a
control signal, following Parisi's interpretation of non-convergence.

Artifacts:

- `results/idea-008-smoke-k3-n80-a4.0/`
- `results/idea-008-k3-n300-a4.15/`

Implementation: `71bd2af`. Results: `a945282`.

## Idea 009 — SP fixed-point branch consensus: killed

Hypothesis: construct an independently initialized SP replica and decimate
only variables whose preferred direction agrees across both fixed points,
ranking by worst-case assignment retention.

The alternate state was measurable. In the N=80 smoke run every replica
converged, mean branch distance was 0.0329, and directional agreement reached
as low as 52.7%. At `N=300`, however, the four failed seeds were nearly
single-branch: mean distance was about 0.003 and mean agreement exceeded
99.6%. The sole successful seed had by far the largest branch distance
(0.0977).

Across five N=300 seeds, outcomes remained 1/5 SAT. Consensus reduced mean
worst drop by 4.9%, but increased move-depth roughness by 1.7%, fixed-depth
roughness by 34.3%, reduced frontier area by 2.0%, and cost 16.7 times more
than certainty.

Decision: kill. Fixed-point multiplicity does not explain the observed
non-convergences on these seeds, and full random restart is too expensive as
an always-on signal. Do not tune the replica count.

Artifacts:

- `results/idea-009-smoke-k3-n80-a4.0/`
- `results/idea-009-k3-n300-a4.15/`

Implementation: `685de8f`. Results: `17faa56`.
