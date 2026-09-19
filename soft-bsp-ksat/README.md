# soft-bsp-ksat

Python port of **survey-inspired decimation (SID)** and **backtracking survey propagation (BSP)** for random K-SAT, starting from the C++ solver in this repository ([Marino, Parisi, Ricci-Tersenghi, Nat. Commun. 7:12996 (2016)](https://www.nature.com/articles/ncomms12996); Parisi [cond-mat/0308510](https://arxiv.org/abs/cond-mat/0308510)).

This package is **step 1 plus the Markovianity diagnostic (point 0)**. There is no Gym environment and no SAC agent yet. Survey propagation remains the only engine for the complexity \(\Sigma\); later RL code must call it rather than learn \(\Sigma\).

## Install and test

Python 3.12+. From this directory:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -e ".[dev]"
pytest
```

CLI (C++-style):

```bash
python src/baseline_bsp.py -w 3 4.0 50 --r 0 --seed 1
python src/baseline_bsp.py -l formula.cnf --r-parisi 0.25
```

## What is ported (off-the-shelf vs custom)

| Piece | Source |
| --- | --- |
| SP equations, \(\Sigma\), CERT scorer \(P(i)=1-\min(s_T,s_F)\), SID/BSP ratio rule | Custom port of `Graph.cpp` / `Vertex.cpp` |
| Residual WalkSAT | Custom compact WalkSAT (same family as `walksat.cpp`, not a line-by-line port) |
| Gym / SAC | **Not present** (blocked on the Markovianity gate) |

Not ported from C++: NN scorer, minisat oracle, `--dataset`, `--theta`, `--veto`, `--damping` (damping stays 0).

## Constants vs C++

| Quantity | C++ | Python default |
| --- | --- | --- |
| \(t_{\max}\) | 1024 | 1024 |
| \(\varepsilon\) | 0.01 | 0.01 |
| ZERO | 1e-10 | 1e-10 |
| \(\rho_{\mathrm{SP}}\) | 1 (SP, not BP) | 1 |
| \(f\) | 0.00125 | 0.00125 (Fig. 1–2 script uses \(10^{-3}\)) |
| \(r\) | 0.9 = \(n_{\mathrm{back}}/n_{\mathrm{dec}}\) | same C++ convention |
| negative-\(\Sigma\) abort | \(-2\) | \(-2\) |
| \(I(k)\) | stale certitude at freeze | same (not recomputed on frozen vars) |

Messages are **warm-started** across moves (not re-randomized), as in C++.

Python `random` is not glibc `random()`. Share DIMACS files for instance-level comparison; do not expect bit-identical cavity messages.

## \(r\) conventions (do not mix)

- **Parisi 2003:** \(r_p = n_{\mathrm{back}}/(n_{\mathrm{back}}+n_{\mathrm{dec}})\), \(r_p<0.5\).
- **C++ / Nature 2016:** \(r_{\mathrm{cpp}} = n_{\mathrm{back}}/n_{\mathrm{dec}}\), \(r_{\mathrm{cpp}}<1\) (code default 0.9).
- Map: \(r_{\mathrm{cpp}} = r_p/(1-r_p)\). Figs. 1–2 values \(\{0,\,0.25,\,0.4\}\) become \(\{0,\,1/3,\,2/3\}\).

\(L\) is the number of **unassigned** variables (Parisi Figs. 1–2 run from \(L\approx N\) down to 0). \(F(L)=\sum |s_T-s_F|/L\) over unassigned variables.

## Reproduce Figs. 1–2

```bash
source .venv/bin/activate
# fast / CI-scale
python experiments/reproduce_fig1_fig2.py --n 1000 --alpha 4.25 --f 0.001 --seed 1
# regression
python experiments/reproduce_fig1_fig2.py --n 10000 --alpha 4.25 --f 0.001 --seed 1
# full-scale (long)
python experiments/reproduce_fig1_fig2.py --n 100000 --alpha 4.25 --f 0.001 --seed 1
```

Qualitative target (Parisi): \(r_p=0\) hits \(\Sigma\to 0\) with \(F\neq 0\) (then collapse); \(r_p>0\) drives \(\Sigma\) and \(F\) to 0 together.

## Markovianity gate (before any RL env)

```bash
python experiments/markovianity.py --n 80 --alpha 4.20 --instances 3
```

The script pairs states with similar \(\varphi=\) (survey summary, \(L\), \(\Sigma\)) reached by **different** freeze histories, continues each with SID, and compares future \(I(k)\) and \(\Sigma\) to a same-history noise floor.

- **Pass:** matched-pair divergence comparable to the noise floor \(\Rightarrow\) \(\varphi\) is usable for `BSPDecimationEnv`.
- **Fail:** systematic, large divergence \(\Rightarrow\) enrich \(\varphi\) with residual-subgraph statistics at **radius 2–3** around each candidate variable, then re-run. Do not start Gym/SAC on an aliased observation.

Output: `output/markovianity/report.json` (`gate_failed`, `verdict`).

## C++ oracle

The parent tree did not build (missing `Logger.hpp` / NN sources). Minimal stubs restore `./build/main` as a comparison oracle (no NN behaviour):

```bash
cmake -S .. -B ../build -DCMAKE_BUILD_TYPE=Release
cmake --build ../build -j
../build/main --r=0 --seed=1 -w 3 2.0 20
```

## Research question (later phases; not run in this package yet)

For \(N\) up to as large as tractable (ideally \(\ge 10^5\)) and \(\alpha\) swept near \(\alpha_c\approx 4.267\) (3-SAT), compare success rate, wall-clock / iteration count, and empirical \(\alpha_A\) for (a) the original fixed-\(r\) baseline at several \(r\) and (b) a learned soft policy. Log \(\Sigma(L)\) and \(F(L)\) against Figs. 1–2. The deliverable is an honest answer to how far treating BSP as SAC / soft Bellman pushes \(\alpha_A\) toward \(\alpha_c\), including negative results. SP stays the only \(\Sigma\) engine.

Planned (blocked on the Markovianity gate):

1. **Two rewards, one optimum.** (a) per-step \(\Delta\Sigma\) (potential-based shaping) and (b) terminal success/failure only. [Ng, Harada, Russell 1999](https://people.eecs.berkeley.edu/~pabbeel/cs287-fa09/readings/NgHaradaRussell-shaping-ICML1999.pdf): shaping \(F=\gamma\Phi(s')-\Phi(s)\) with \(\Phi=\Sigma\) leaves the optimal policy invariant. Any gap is **learning efficiency**, not a different objective.
2. **\(\lambda\) is not Parisi’s \(x\).** Treat SAC temperature as a free hyperparameter; find \(\lambda^*(\alpha)\) empirically first. Only afterwards check whether \(\lambda^*(\alpha)\) correlates with theoretical 1RSB \(x(\alpha)\). Report that correlation (or the lack of it) as a **result**, not a design assumption.

## Layout

```
soft-bsp-ksat/
  src/sat_instance.py
  src/survey_propagation.py
  src/baseline_bsp.py
  src/walksat.py
  experiments/reproduce_fig1_fig2.py
  experiments/markovianity.py
  tests/
```
