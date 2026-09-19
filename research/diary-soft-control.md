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
- [x] Pilot K4 N=1000 a={9.5} seeds=1-5, stessi configs.
- [x] Tabella baseline α_a(N) preliminare (sotto).

### Baseline preliminare (seeds 1-5, r=0.9)

| Cella | cert | pol | gamma:0.01 | dynI |
|---|---|---|---|---|
| K3-N300-a4.0 | 2/5 | 4/5 | 3/5 | 4/5 |
| K3-N300-a4.15 | 1/5 | 1/5 | 1/5 | 1/5 |
| K4-N1000-a9.5 | 2/5 (3 sp-nc) | 3/5 (2 sp-nc) | 2/5 (3 sp-nc) | 3/5 (1 sp-nc + 1 ws-fail) |

K4-dettagli: dynI ha la roughness più bassa (0.0321 vs 0.0386 cert) ma wall medio
62s vs ~25s per una coda walksat da 150s (seed 4). pol risolve 3/5 con meno passi
medi (7236 vs 9036 cert): più aggressivo, più ruvido (0.0414).

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
- E0/K3 confermano prior leaderboard → harness valido, si procede.
- N=300 non separa le policy a 4.15: pilot decisionali a N>=1000 (K3), N=1000 (K4).

## E1 — Caratterizzazione del crollo (risultati preliminari K3, branch `cursor/soft-e1-collapse-67c3`)

Strumento: `tools/collapse_signatures.py` (AUC di firma-su-prefisso → fallimento finale).

Risultati (20 run/cella, truncation frazionaria):
- **eta_max (max iterazioni SP nel prefisso): AUC 0.92 (a=4.0) e 1.00 (a=4.15)
  già a f=0.1.** Lo sforzo di convergenza SP predice il destino quasi subito.
  → segnale di stabilità (energia candidata d) confermato come early warning.
- **sigma_frac (Σ_trunc/Σ_0): AUC 0.76→0.91 (a=4.0), 0.84→1.00 (a=4.15).**
  Il livello di Σ trattenuta è predittivo, ma più tardi di eta.
  → energia (a) porta informazione, ma eta arriva prima.
- drop_max/roughness: deboli a 4.0 early (AUC~0.4-0.5), forti a 4.15 late (~0.9):
  la ruvidezza di Σ è firma tardiva, non preallarme.
- slope (discesa media): ~chance. Separano gli estremi, non la media.
- back_frac: rumoroso.

### Feedback / decisioni
- Implicazione per il controllo: un trigger eta-based per attivare policy soft
  (E3/E4) solo quando SP fatica è ora motivato empiricamente, non solo teorico.
- Caveat: n=20/cella; confermare su K4 + più seed prima di fissare soglie.
- Prossimo: estendere firme a ρ̂ (contrazione messaggi) se loggabile a costo zero.

## E2 — Oracolo ΔΣ 1-passo (port completato, pilot da lanciare, branch `cursor/soft-e2-oracle-67c3`)

- Port di 1a933cf (idea-001) sull'harness dynamic-I: cherry-pick + risoluzione
  conflitti banali (tenute entrambe le chiavi diag). Build OK.
- Smoke N=80: ASSIGNMENT FOUND con --lookahead-k=4. Il flag mancava nel branch
  final-dynamic-I (solo dichiarazione in Header.h), ora funzionante.
- Pilot previsto: K3-N1000-a4.15 + K4-N1000-a9.5, seeds paired vs cert, L=1.
  Atteso costoso (~100x): lanciare dopo chiusura E0/K4 per non contendere CPU.

## E3 — Boltzmann su score esistenti (implementato, sweep in corso, branch `cursor/soft-e3-boltzmann-67c3`)

- Implementazione: `boltzmann_pick()` + `--temperature=T --shortlist=K`
  (default K=10), L=1, direzione da regola SP, RNG seedato.
- Verifica: T=0 produce curva Σ **bitwise-identica** al legacy (K3-N300-a4.0 seed 1).
- Sweep in corso (tmux soft-e3-sweep): K3-N300-a4.15 seeds 1-5,
  T ∈ {0.005, 0.01, 0.05, 0.1, 0.5} vs greedy.
- Nota di disegno: score cert/pol in [0,1] → T~0.01-0.1 è la scala interessante;
  T=0.5 è controllo "quasi-uniforme", ci si attende degrado.
- 2026-09-19 sweep K3-N300-a4.15: TUTTI 1/5 come greedy (null atteso a N=300).
- 2026-09-19 sweep K3-N1000-a4.15 seeds 1-5: greedy 5/5 (soffitto, nessun headroom),
  T001 4/5 (1 walksat-fail + coda 103s), T005 5/5, T01 5/5. Nessun guadagno, T alza
  leggermente roughness. Prossimo: cella più dura (a=4.2 o seeds 6-15) dove greedy
  fallisce, per testare se la softness salva.

## E4+ — Rollout Gibbs con energie alternative (disegnato, non avviato)

Confronto diretto energie (a) vs (c-surrogate) vs (d) come costo di rollout H=2-5.
Richiede E2>0 oppure E1 con AUC>0.7, altrimenti riconsiderare.
