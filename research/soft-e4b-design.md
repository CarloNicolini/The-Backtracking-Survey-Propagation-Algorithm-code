# E4b design — Gibbs H-step rollout on eta-triggered fragile points

Branch: `cursor/soft-e4-energy-67c3` (continues E4a). Status: designed, not implemented.
Depends on: E4a pilot (trial logs calibrate trigger + show whether H=1 suffices).

## Why

E2/E4a test one-step energies. If H=1 does not separate (or separates weakly),
the information may live in trajectory ensembles (Kappen / KL-control):

$$P^*(\tau) \propto P_0(\tau)\,\exp(-S[\tau]/\lambda),
\qquad S[\tau]=\sum_{h=0}^{H-1} \ell(x_{t+h},a_{t+h})$$

E1 motivates an eta trigger: intervene only when SP itself signals fragility,
which also makes the cost affordable.

## v1: depth test with deterministic greedy rollouts (M=1)

- Reference $P_0$: greedy cert decimation (backtrack schedule unchanged outside rollouts;
  rollouts themselves are decimation-only, depth H=2..3).
- Trigger: decimation step with `eta_last >= T_eta` and Sigma > 0. Calibrate
  `T_eta` from E4a `*_trials.csv` (eta_after distribution on failing vs
  succeeding runs). Target trigger rate < 15% of steps.
- At triggered step: 8 trials (top-4 x 2 dirs). Forked child fixes (v,dir),
  reconverges (cost $c_0$, $C=\infty$ if SP dies), then H-1 greedy
  fix-top-1 + reconverge cycles, accumulating $c_h$. Guard `_in_rollout`
  forces greedy inside (no recursive lookahead).
- Step energies compared: $\ell^a=-\Delta\Sigma$ (Sigma-drop),
  $\ell^d=\eta_{after}$, $\ell^{hyb}$ (E4a gate). Terminal fatality: hard
  gate ($C=\infty$) first; finite-penalty ablation later.
- Decision: argmin $C$ (M=1, so $\lambda$ is vacuous in v1 — v1 isolates the
  depth effect H>1 vs H=1).
- Controls: (i) same trigger + H=1 (must reproduce E4a); (ii) random candidate
  at triggered steps (is any deviation good, or must it be informed?);
  (iii) greedy baseline. Paired seeds.
- Metrics: $p_{succ}$, rescues (greedy-fail→sat), kills (greedy-sat→fail),
  trigger rate, wall/success.
- Kill: no rescues on K3-N1000-a4.15/a4.2 seeds 1-5, or rescues ~= kills.

## v2: Gibbs proper (M>1, lambda sweep)

- Merge E3 `--temperature` into rollout policy: M=4 stochastic rollouts per
  candidate, $Q=-\lambda\log\langle e^{-C/\lambda}\rangle$.
- Sweep $\lambda$: small (risk-sensitive, best-trajectory) → large (mean).
  $\lambda\to\infty$ must approach the mean-cost rule (sanity check).
- This is the direct test of the Gibbs-trajectory hypothesis. Requires v1 to
  show depth matters (H>1 beats H=1 at fixed energy).

## Finite-y energy (c): status and paths

NOT in v1/v2 until messages are verified. Two paths:

- **C1 — rho_SP lead.** `__norm()` already interpolates SP↔BP via `rho_SP`
  (1=SP, 0=BP per comment), but the complexity formula is inconsistent for
  rho≠1 (variable term has no rho). Verification plan: (1) derive correct
  SP(m) message + free-entropy formulas from Mezard-Parisi-Zecchina /
  Braunstein-Zecchina; (2) check rho=0 fixed point against independent BP;
  (3) check m-limit recovers SP Sigma. If verified, trial energy = replicated
  free entropy at finite m — the cheapest route to (c).
- **C2 — 4-state forcing-bit messages**, per idea-025 conclusion (supported-
  variable bias most sign-consistent). Real project, larger blast radius.

## Cost estimate (v1)

Per triggered step: 8 trials x H SP runs ≈ 8x3x20ms ≈ 0.5s. At 10% trigger
rate over ~8000 steps: ~400s/run overhead. Acceptable for seeds 1-3 pilots.
