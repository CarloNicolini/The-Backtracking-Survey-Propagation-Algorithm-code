# Diario — Soft Control per BSP (KL-control / path-integral)

Programma: spostare $\alpha_a$ di BSP su K-SAT (K=3,4) vicino alla soglia SAT-UNSAT
tramite policy di decimazione/backtracking soft alla Kappen/Levine.
Branch base: `cursor/soft-control-harness-67c3` (da `origin/research/idea-025-biased-enumeration`).

## 0. Cornice teorica concordata (2026-09-19)

Struttura di controllo: MDP + soft Bellman. Lasso formale:

$$P^*(\tau) \propto P_0(\tau) \exp\left[-\frac{1}{\lambda}\sum_t \ell_{SP}(x_t,a_t)\right]$$

dove $P_0$ è la legge della policy di riferimento (BSP greedy) e $\ell_{SP}$ è
l'**energia di traiettoria da determinare**, NON fissata a priori a $-\Sigma$.

Candidati per $\ell_{SP}$ (questione scientifica aperta):
1. **(a) $-\Sigma(x_t)$** — complessità 1RSB a $y\to\infty$ (n. cluster). Scelta naturale, ma proiezione incompleta.
2. **(b) $-\log \mathcal N_{cl}$ / conteggi** — varianti microcanoniche (quasi-equivalenti ad (a), da non duplicare).
3. **(c) $-F(y; x_t)/y$ free-entropy a $y$ finito** — ensemble già termodinamicamente regolarizzato; pesa cluster per entropia interna. Ipotesi favorita dal PI. Richiede rappresentazione messaggi oltre lo scalare SP (vedi idea-025: surrogate a 2/4 stati).
4. **(d) stabilità del punto fisso SP** — $\hat\rho = d_t/d_{t-1}$, $\eta_{conv}$, Lyapunov/adjoint (idee 011/016/023: segnali deboli ma non combinati con controllo soft).

Protocollo decisionale: E1 misura il potere predittivo di (a),(d) sui fallimenti
con i log esistenti; E2 misura il guadagno dell'oracolo $\Delta\Sigma$ (upper bound
di (a) a 1 passo); E3 isola l'effetto softness pura; E4 (rollout Gibbs) è il primo
posto dove (c) e (d) entrano come energie alternative a confronto diretto con (a).

Precedenti rilevanti dai branch research (non ripetere):
- idea-001 lookahead-$\Sigma$ esatto: kill (118x costo, nessun salvataggio a N=300/a=4.15).
- idea-002 margine relativo: kill (inattivo vicino alla soglia).
- idea-003 gamma:0.01: winner K3-target (soft polarization, costo zero).
- idea-006 dynamic-I backtrack: winner K3-target + K4-9.5 (rilasci per $I(k)$ corrente).
- idee 011/012 stabilità: segnale debole come trigger; mai testato come energia Gibbs.
- idea-025 biased enumeration: surrogate $b^{V_1}t^{C_1}$ passa il gate meccanico a N=20;
  supporta modello a 4 stati per (c). ESS alto (0.88-0.99), effetto piccolo ma consistente.

## Worktree

| Esperimento | Branch | Worktree | Stato |
|---|---|---|---|
| E0 harness+baseline | `cursor/soft-control-harness-67c3` | `/workspace` (principale) | in corso |
| E1 firme del crollo | `cursor/soft-e1-collapse-67c3` | `/tmp/soft-wt/e1` | da creare |
| E2 oracolo ΔΣ | `cursor/soft-e2-oracle-67c3` | `/tmp/soft-wt/e2` | da creare |
| E3 Boltzmann | `cursor/soft-e3-boltzmann-67c3` | `/tmp/soft-wt/e3` | da creare |

Regola: ogni worktree compila in `build-<exp>` proprio; i risultati grezzi vanno in
`results/soft-<exp>-*/` sul branch dell'esperimento; il diario resta sul branch harness
e viene mergiato/aggiornato a ogni chiusura esperimento.

## E0 — Harness + baseline (in corso)

- [x] 2026-09-19: base scelta = idea-025 (C++ identico a final-dynamic-I + tools biased).
- [x] 2026-09-19: build OK con `CXX=g++-13` (clang di default non linka libstdc++).
- [x] 2026-09-19: smoke `--diag` OK (steps/moves/vars CSV).
- [x] Pilot K3 N=300 a={4.0,4.15} seeds=1-5, configs cert/pol/gamma:0.01/dynamic-I.
- [ ] Pilot K4 N=1000 a={9.5} seeds=1-5, stessi configs.
- [ ] Tabella baseline α_a(N) preliminare.

### Osservazioni
- N=50 smoke mostra `ERROR - incorrect problem format` in walksat su residuo vuoto:
  artefatto taglia piccola, non blocco (verificare sparisca a N>=300).
- `tools/eval_complexity.py` riusato come runner (già classifica sat/sp-nonconvergence/
  contradiction/negative-sigma/timeout + fissa --diag-every enorme per vars).
- 2026-09-19 E0/K3-N300-a4.0 (seeds 1-5): cert 2/5 sat, pol 4/5, gamma001 3/5,
  dynI 4/5. Riproduce leaderboard idea-002/003 (cert 2/5). Nota: un seed cert ha
  wall anomalo (~77s, walksat su residuo duro) → media wall cert 15.8s vs ~0.3s
  altri. Da fissare timeout walksat o cutoff per E7 (costo pari budget).
- 2026-09-19 E0/K3-N300-a4.15 (seeds 1-5): tutti 1/5 sat, 4/5 sp-nonconvergence.
  Nessuna separazione tra policy a N=300 vicino alla soglia → conferma che N=300
  è troppo piccolo per decidere (effetto taglia finita, come da leaderboard:
  servono N>=1000 per K3). Curve Σ: mean_abs_delta ~0.013-0.016, max_drop ~0.45.

### Feedback / decisioni
- (da compilare dopo i pilot)

## E1 — Caratterizzazione del crollo (prossimo)

Obiettivo: AUC di (firme a t → fallimento). Firme: dΣ/dt, η_conv, frazione low-margin,
ρ̂ se loggabile a costo zero. Output: soglia critica operativa per attivare E3/E4.

## E2 — Oracolo ΔΣ 1-passo (prossimo)

Riuso `--lookahead-k` (idea-001) ma: N=1000 (non 300), batch L=1, confronto paired.
Domanda: upper bound di energia (a) a 1 passo. Kill se Δp_succ ≈ 0.

## E3 — Boltzmann su score esistenti (prossimo)

Nuovo flag `--temperature=T --shortlist=K`: π(i) ∝ exp(score_i/T).
Sweep T logaritmico + limiti T→0 (regressione) e T→∞ (controllo negativo).
Prima solo scelta variabile a L=1, poi direzione. Domanda: quanta parte del
problema è "durezza" vs "score sbagliato".

## E4+ — Rollout Gibbs con energie alternative (disegnato, non avviato)

Confronto diretto energie (a) vs (c-surrogate) vs (d) come costo di rollout H=2-5.
Richiede E2>0 oppure E1 con AUC>0.7, altrimenti riconsiderare.
