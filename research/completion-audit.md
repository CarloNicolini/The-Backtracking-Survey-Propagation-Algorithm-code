# Completion audit

## Winning mechanism

The final branch is `research/final-dynamic-I`. Commit `6843f8c` exposes the
winner as `--dynamic-i-backtrack`. Its mechanism and derivation are documented
in `research/dynamic-i-backtracking.md`.

The implementation reconstructs current cavity warnings for fixed variables,
computes assignment-specific Parisi `I(k)`, and ranks standard BSP releases by
minimum current `I(k)`. Defaults remain unchanged when the flag is absent.
The clean branch excludes the killed exchange, transaction, compound, replica,
stability-trigger, basin-jump, and regret controllers.

## Estimator evidence

On 120 fork-isolated release/reconvergence trials at K=3, N=300, predicted
release gain `-log(I(k))` had Pearson correlation 0.99684 with measured
complexity gain, all signs were correct, and MAE was 0.00056.

## K=3 target evidence

Random 3-SAT, N=10000, alpha=4.2, seeds 1–5:

- dynamic-I: 5/5 SAT;
- certainty: 4/5 SAT;
- polarization: 5/5 SAT;
- gamma=0.01 contender: 5/5 SAT.

Against certainty, dynamic-I reduced mean signed complexity descent by 26.0%,
mean absolute DeltaSigma by 12.1%, fixed-depth roughness by 17.7%, and
fixed-depth maximum drop by 17.9%, at 1.058 times wall time. It had the lowest
signed descent and fixed-depth roughness of all four policies. Gamma retained
a 5.8% advantage in mean absolute DeltaSigma; dynamic-I reduced gamma's
fixed-depth maximum drop by 56.3%.

Evidence:

- `results/idea-006-k3-n10000-a4.2-5seeds-comparison/summary.tsv`
- `results/idea-006-k3-n10000-a4.2-5seeds-comparison/aggregate.tsv`
- `results/idea-006-k3-n10000-a4.2-5seeds-comparison/sigma_curves.svg`
- `results/idea-006-k3-n10000-a4.2-5seeds-comparison/sigma_vs_fixed.svg`

## K=4 target evidence

Random 4-SAT, N=1000, alpha=9.5, seeds 1–10:

- dynamic-I: 6/10 SAT;
- certainty: 5/10 SAT;
- polarization: 6/10 SAT.

Against certainty, dynamic-I reduced mean signed descent by 23.2%, mean
absolute DeltaSigma by 8.6%, fixed-depth roughness by 4.8%, and fixed-depth
maximum drop by 39.8%; frontier area/level rose 3.7%. Wall time was 1.94
times certainty. At alpha=9.7 all policies failed 0/5, so no higher alpha
ceiling is claimed; dynamic-I still gave the smoothest trajectory.

Evidence:

- `results/idea-006-k4-n1000-a9.5-10seeds/summary.tsv`
- `results/idea-006-k4-n1000-a9.5-10seeds/aggregate.tsv`
- `results/idea-006-k4-n1000-a9.5-10seeds/sigma_curves.svg`
- `results/idea-006-k4-n1000-a9.5-10seeds/sigma_vs_fixed.svg`
- `results/idea-006-k4-n1000-a9.7-5seeds/`

## Correctness and reproducibility

- All 15 target seed pairs used by certainty and dynamic-I had identical
  generated-CNF SHA-256 hashes.
- All five K=3 dynamic-I SAT assignments independently satisfy all 42,000
  clauses.
- All six K=4 dynamic-I SAT assignments independently satisfy all 9,500
  clauses.
- The clean final branch reproduces the research branch's K=4 seed-1 numeric
  trajectory exactly over 10,750 logged fixed points.
- Default certainty behavior reproduces the original N=80 seed-1 trajectory.
- `bsp` and `bsp-train` build successfully with AppleClang locally and GCC
  9.4 on `muletto`; the analysis script compiles under Python.

## Research trace

Every tested mechanism has a dedicated `research/idea-*` branch/worktree and
implementation commit. `research/leaderboard.md` records hypotheses,
decisions, metrics, artifact paths, and the reasons each losing controller was
killed or retained only as a component.

The evidence supports dynamic-I as the balanced target-scale winner. It
improves success or matches the best baseline, improves the central smoothness
metrics at both requested physical scales, costs 1.94 times certainty at the
K=4 target and 1.06 times at K=3, and uses a parameter-free mechanism derived
from Parisi's current `I(k)`.
