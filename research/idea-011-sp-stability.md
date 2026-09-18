# Idea 011 — SP contraction as a stability certificate

## Hypothesis

Always-on transaction probes rescue failures but cost too much. Trigger them
from the stability of the SP fixed point itself. During each SP iteration,
record the maximum clause-to-variable message change `d_t`. At convergence,
define

`rho_hat = d_t / d_(t-1)`.

This is a zero-cost empirical contraction factor for the current update map.
A value approaching one indicates weak attraction and approximates the
spectral-radius warning expected near a spinodal or competing fixed point.
Unlike iteration count, it measures local dynamics directly and introduces no
learned score.

The first experiment is diagnostic only. It logs the accepted residual and
contraction factor without changing any move, then asks whether fatal
decimations/backtracks are preceded by systematically weaker contraction than
ordinary moves. If the signal separates them, the transaction-safe recovery
from idea 010 will be activated only when SP's own stability certificate says
the parent fixed point is fragile.

## Falsification

Kill the signal if fatal-action checkpoints have contraction values
indistinguishable from the run background, or if computing the full maximum
residual changes baseline trajectories. Promote only after verifying bitwise
identical Sigma curves and outcomes with the new diagnostics enabled.
