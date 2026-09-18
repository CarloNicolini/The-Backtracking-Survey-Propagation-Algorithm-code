# Idea 012 — stability-triggered transaction safety

## Hypothesis

Transaction isolation rescues fatal BSP moves, but probing every action is too
expensive. Use SP's own convergence effort as an endogenous trigger.

Before adding the current fixed point to history, compute

`eta_rms = sqrt(mean(previous eta^2))`.

If current `eta > eta_rms`, transactionally probe the scheduled decimation or
backtracking move and invoke the recovery ladder when necessary. Otherwise
execute the exact legacy action without a probe. This threshold is determined
entirely by the run's prior dynamics; no cutoff is fitted.

On development seeds 1–5, the contraction-ratio hypothesis failed, but eta
discriminated fatal checkpoints from ordinary states with AUC 0.976. The RMS
rule caught all four fatal checkpoints while activating on 14–40% of states.
The experiment is preregistered on unseen seeds 6–15.

## Predictions

- Before its first triggered state, each run is exactly the certainty baseline.
- Fatal moves caught by the RMS trigger should be rejected or repaired.
- Trigger frequency and wall overhead should remain far below always-on
  transaction safety.
- Smoothness should remain close to certainty except on trajectories extended
  by successful recovery.

The option is `--stability-safe`. Kill the rule if held-out fatal moves are
missed, success does not improve, or overhead still exceeds three times
certainty on normal paths.
