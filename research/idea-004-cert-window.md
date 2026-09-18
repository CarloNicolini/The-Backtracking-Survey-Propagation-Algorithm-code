# Idea 004 — certainty-windowed gamma ranking

## Hypothesis

`gamma=0.01` reduces average `|Delta Sigma|` and improves target-scale
success, but its worst drop is larger than certainty's. Limit that tail by
first sorting unfixed variables by certainty, then applying the gamma score
only inside the top-K certainty window. This prevents directional
regularization from selecting a variable far outside the low-immediate-cost
region.

The option is `--cert-window=K`; zero retains the existing global scorer. The
window is automatically enlarged when necessary to contain the full
decimation batch. It adds sorting work but no SP reconvergences.

## Pilot protocol

Run random 3-SAT at `N=1000`, `alpha=4.15`, `r=0.9`, seeds 1–5. Compare
certainty, polarization, global `gamma=0.01`, and gamma restricted to certainty
windows of 8, 32, and 128 variables.

Keep a window only if it preserves gamma's convergence and roughness gains
while reducing its maximum pre-terminal drop toward certainty. Promote the
best window to `N=10000` and test it on the same target formulas used for idea
003.
