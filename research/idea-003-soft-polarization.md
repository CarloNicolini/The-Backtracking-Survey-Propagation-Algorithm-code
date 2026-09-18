# Idea 003 — soft absolute-polarization regularization

## Hypothesis

Certainty approximately preserves immediate complexity but can prefer
high-`sI` variables whose direction is determined by a tiny absolute survey
difference. Polarization gives stronger directions but produced rougher Sigma
curves in the first two pilots. Interpolate with

`score(i) = certainty(i) * |sT(i) - sF(i)|^gamma`,

using small positive `gamma`. This directly suppresses low-information
directions while retaining more of certainty's complexity-preserving ranking
than pure polarization. The implementation already exists behind
`--scorer=gamma:G`, so no solver change is required.

## Pilot protocol

Repeat the idea-002 conditions: random 3-SAT, `N=300`, `r=0.9`, seeds 1–5,
at `alpha=4.0` and `4.15`. Compare certainty, polarization, and `gamma` in
`{0.05, 0.1, 0.25, 0.5}`.

Promote a gamma only if it improves convergence/success while keeping the
maximum drop and mean absolute delta close to certainty, with no material
wall-clock overhead.
