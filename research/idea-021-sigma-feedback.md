# Idea 021 — parameter-free Sigma feedback scheduling

## Hypothesis

Replace the fixed backtracking ratio with feedback from the realized
complexity response. After every decimation, compare its positive drop with
the running mean of prior decimation drops:

- if the new drop exceeds the prior mean, schedule one backtrack;
- otherwise continue decimating;
- after every backtrack, force a decimation to guarantee progress.

The running mean is updated online and supplies its own scale. There are no
PID gains, target slope, window length, or fitted threshold. Variable
selection and release ordering remain the certainty baseline, isolating the
scheduling mechanism.

## Predictions

- Backtracking should occur near accelerating complexity loss rather than at a
  fixed cadence.
- Mean signed and absolute DeltaSigma should fall while using fewer wasted
  releases on the plateau.
- SP non-convergence should decrease if fixed-ratio BSP reacts too late to
  anomalous drops.

The option is `--sigma-feedback`. Compare against certainty and polarization
at identical seeds; kill if progress becomes too aggressive or success falls.

## Result

The first N=300 seed entered a deterministic two-cycle: 874,688 fixed points,
backtracking fraction 0.5, only one net fixed variable, and no termination
after more than 200 seconds. The controller reacts to its own repair gain:
decimation exceeds the running mean, backtracking undoes it, and the forced
decimation repeats the same state.

Decision: kill immediately without running additional seeds. A controller on
the first derivative of Sigma needs hysteresis or a state-space model; a
memoryless above-mean switch cannot guarantee net progress.
