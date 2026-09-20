# DRAFT — Soft-Control Decimation for Random \(K\)-SAT: A Path-Integral Approach to Survey Propagation

*Draft theory note, 2026-09-19. Style: Phys. Rev. E. Status: working document, not submitted.*

---

## Abstract

We formulate the decimation stage of survey-propagation-based solvers for random
\(K\)-SAT as a stochastic optimal-control problem and propose to replace the
deterministic greedy heuristics in current use (certainty, polarization) with
soft policies derived from Kullback-Leibler (path-integral) control and maximum-entropy
reinforcement learning. The central object is a Gibbs measure over decimation
trajectories,
\(P^\star(\tau) \propto P_0(\tau)\exp(-S[\tau]/\lambda)\),
where \(P_0\) is a reference (greedy) policy and \(S[\tau]=\sum_t \ell(x_t,a_t)\)
is a trajectory action whose local cost \(\ell\) — the *trajectory energy* — is left
open. We argue that the 1RSB complexity \(\Sigma\) (log-number of clusters) is only
one candidate for \(\ell\), and possibly an incomplete projection of the relevant
state near the SAT-UNSAT threshold; alternatives include survey-propagation stability
observables (convergence effort, contraction rate) and finite-\(y\) (finite-\(m\))
1RSB free entropies that reweight clusters by internal entropy. Selecting the
trajectory energy that best predicts search survival is posed as a joint theoretical
and empirical question. We describe a staged experimental program on 3-SAT and 4-SAT
near threshold, report preliminary diagnostics, and state sharp falsification criteria
for each hypothesis.

---

## I. Introduction

Random \(K\)-satisfiability (random \(K\)-SAT) is the canonical constraint-satisfaction
problem with a rich thermodynamic structure: as the clause density
\(\alpha = M/N\) grows, the solution space shatters into exponentially many clusters
(dynamical/clustering transition \(\alpha_d\)), condenses onto a subexponential number
of dominant clusters (\(\alpha_c\)), and finally disappears at the SAT-UNSAT threshold
\(\alpha_s\) [1–4]. Message-passing algorithms built on the cavity method — notably
survey propagation (SP) with decimation (SID) and its backtracking extension (BSP) [5–8] —
are among the few solvers that penetrate the clustered (hard-SAT) phase, yet their
algorithmic threshold \(\alpha_a\) still lies strictly below \(\alpha_s\), and the gap
widens in practice as instances approach the threshold, where SP fixed points become
fragile and decimation errors fatal.

A long-standing intuition, emphasized by Parisi in the context of BSP, is that a better
algorithm should *slow down the descent of the complexity* \(\Sigma\), i.e. kill
solution clusters as slowly as possible while decimating, thereby keeping the residual
search in regions where many clusters — and hence many options — survive. Close to the
threshold the number of clusters varies rapidly along decimation, clusters become
strongly correlated, and early ("seed") decisions determine the fate of the run. The
practical question is: which local rule selects, at each step, the variable and direction
that best preserve the future of the search?

Current rules are deterministic functions of the single-site surveys \((s_T,s_F,s_I)\):
the *certainty* \(1-\min(s_T,s_F)\) and the *polarization* \(|s_T-s_F|\). At each step
the algorithm ranks all unfixed variables and fixes a top batch. These scores are, in the
language of control theory, *hard* (zero-temperature \(\arg\max\)) policies over a
*global* ranking. They quantify neither uncertainty about the ranking nor the future
consequences of a move, and they ignore correlations between co-decimated variables.
We argue that this hardness is a plausible bottleneck near the threshold, and that the
natural replacement is a *soft* (entropy-regularized) policy grounded in optimal control.

The contribution of this note is (i) a clean mapping of SP decimation onto KL-regularized
optimal control, unifying the Kappen path-integral view [9,10], linearly-solvable MDPs
[11], and maximum-entropy (soft actor-critic) reinforcement learning [12–14]; (ii) an
explicit separation between the *control formalism*, which we take as fixed, and the
*trajectory energy* \(\ell\), which we leave as the open physical question; (iii) a
candidate list for \(\ell\) spanning bare complexity, SP-stability observables, and
finite-\(y\) free entropies; and (iv) a staged experimental program with falsifiable
predictions, with preliminary results.

## II. Background: SP, complexity, and decimation

We recall only what the control formulation needs. Consider a \(K\)-SAT formula with
\(N\) variables and \(M=\alpha N\) clauses, represented as a factor graph. Survey
propagation iterates warnings \(\eta_{a\to i}\) (probability that clause \(a\) warns
variable \(i\), i.e. requests a specific value) to a fixed point; from it one computes,
per variable, the surveys \(s_T,s_F,s_I\): the fraction of clusters in which the variable
is forced true, forced false, or free. The 1RSB complexity,

\[\Sigma = \Sigma_{\mathrm{clauses}} - \Sigma_{\mathrm{variables}},\]

estimates the logarithm of the number of solution clusters [2,5]. Decimation fixes a
variable (typically toward \(\arg\max(s_T,s_F)\)), simplifies the formula, and repeats
SP to a new fixed point. BSP [8] adds stochastic backtracking steps (ratio \(r\)) that
release weakly-supported variables. The run ends in the paramagnetic phase
(\(\Sigma\to 0\), residual solved e.g. by WalkSAT), or fails (SP non-convergence,
contradiction, negative \(\Sigma\)).

Two facts matter. First, \(\Sigma(t)\) along decimation is monotone-ish far from the
threshold but falls steeply near it: the process burns through clusters rapidly exactly
where caution is most needed. Second, SP fixed points near the threshold are marginally
stable: convergence slows, and small perturbations (one wrong fixation) push the dynamics
out of the basin. Both observations point to the same need: decisions should account for
*robustness of the future*, not just local survey margins.

## III. The hardness bottleneck

Write the decimation rule as a policy \(\pi(a|s) = \delta(a-\arg\max_a Q_{\mathrm{hard}}(s,a))\)
with \(Q_{\mathrm{hard}}\) the certainty or polarization score. Three limitations follow:

1. **No uncertainty.** \(|s_T-s_F| = 10^{-3}\) and \(|s_T-s_F| = 0.4\) both produce a
   direction; the ranking does not know that the first is a coin flip. Batch decimation
   compounds such flips.
2. **No lookahead.** The score is a function of the *current* fixed point only. Two
   variables with equal scores may lead one step later to a healthy SP fixed point and
   to non-convergence, respectively; the rule cannot distinguish them.
3. **No correlation awareness.** Co-decimated variables share clauses; their joint effect
   on cluster survival is not the sum of marginal scores. Near the threshold, where
   clusters are strongly correlated, this mean-field-over-moves approximation breaks.

Smoothing the score (e.g. \(b\cdot|s_T-s_F|^\gamma\)) mitigates (1) but not (2)–(3).
A principled fix requires valuing *moves by their futures*, i.e. optimal control.

## IV. Decimation as KL-regularized optimal control

### A. MDP formulation

Define the Markov decision process: state \(x_t\) = (residual formula, SP fixed point,
\(\Sigma_t\)); action \(a_t\) = (variable, direction) or a backtracking release; local
cost \(\ell(x_t,a_t)\); terminal cost \(0\) on solve, \(+\infty\) (or large penalty) on
SP death/contradiction. Let \(P_0\) be the reference (greedy BSP) trajectory law. The
KL-control objective [9–11] is

\[\min_P\ \mathbb{E}_P[S(\tau)] + \lambda\, D_{\mathrm{KL}}(P\|P_0),
\qquad S(\tau) = \sum_t \ell(x_t,a_t),\]

whose optimizer is the Gibbs (twisted) ensemble over trajectories,

\[\boxed{P^\star(\tau) \propto P_0(\tau)\,
\exp\!\left[-\frac{1}{\lambda}\sum_t \ell(x_t,a_t)\right].}\]

For \(\lambda\to 0\) one recovers greedy optimal control; for \(\lambda\to\infty\),
the reference policy. Intermediate \(\lambda\) yields risk-sensitive interpolation:
trajectories are reweighted by plausibility under \(P_0\) times Boltzmann weight of
their cost. The associated dynamic programming is the *soft Bellman equation* [12–14]:

\[Q(s,a) = \ell(s,a) + \gamma\,\mathbb{E}_{s'}[V(s')],\qquad
V(s) = -\lambda\log\sum_a \pi_0(a|s)\,e^{-Q(s,a)/\lambda},\]

with optimal policy \(\pi^\star(a|s)\propto \pi_0(a|s)\,e^{-Q(s,a)/\lambda}\).
Sampling the next move from \(\pi^\star\) — rather than \(\arg\max\) — is the precise
sense in which Kappen's path-integral control [9] ("estimate the move by sampling
Gibbs-weighted trajectory ensembles") and soft actor-critic practice coincide here.

### B. What the formalism fixes — and what it leaves open

The formalism resolves §III structurally: softness handles (1), trajectory ensembles
handle (2), and joint trajectory costs can in principle capture (3). But it does *not*
specify \(\ell\). This is deliberate, and it is the core scientific question of the
program: **which observable best represents the future health of a SAT search?**
We enumerate candidates next. The choice is not a mere implementation detail: optimizing
trajectories under the wrong energy is worse than useless, as it concentrates sampling on
futures that look good under a proxy while dying under the true dynamics.

## V. Candidate trajectory energies

**(a) Bare complexity, \(\ell^a_t = -\Sigma(x_t)\).** The Parisi criterion in its simplest
form: reward staying in high-complexity regions, i.e. minimize cluster killing per step.
It is natural, cheap (one SP reconvergence per candidate move), and directly operationalizes
"slow the descent of \(\Sigma\)". Its weakness is conceptual: \(\Sigma\) is a scalar,
microcanonical/geometric projection (log-count of clusters at \(y\to\infty\)); two
residual formulas with equal \(\Sigma\) can have very different internal cluster-entropy
distributions and SP-stability properties. Near the threshold, where the relevant
differences between futures may lie precisely in those hidden coordinates, \(\ell^a\) may
optimize a shadow.

**(b) SP-stability observables, \(\ell^d\).** Quantities read off the SP dynamics itself:
per-step convergence effort \(\eta\) (iterations to fixed point), empirical contraction
factor \(\hat\rho = d_t/d_{t-1}\) of message residuals, or Lyapunov/adjoint sensitivities
of the fixed point. Motivation: SP death, the dominant failure mode, is preceded by
weakening attraction; moves that keep SP "comfortably converged" (low \(\eta\), strong
contraction) plausibly preserve cluster diversity as a side effect. These observables are
microscopic (they see the message ensemble, not just its \(\Sigma\) projection) and nearly
free given trial reconvergences. Their weakness: stability is a property of the *algorithm's*
dynamics, not of the *instance's* thermodynamics; the link "stable SP \(\Rightarrow\)
surviving clusters" is plausible but not a theorem.

**(c) Finite-\(y\) (finite-\(m\)) free entropy.** The 1RSB ensemble at finite Parisi parameter
reweights clusters by internal entropy, \(\propto e^{y N s_\alpha}\); \(\Sigma\) is the
\(y\to\infty\) (all clusters equal) limit, while \(y\to 0\) recovers solution counting.
A trajectory energy built on the finite-\(y\) free entropy \(\Phi(y)\) would control the
search with a quantity that is *already thermodynamically regularized*, instead of applying
Gibbs reweighting on top of the bare cluster count. This is conceptually the most
satisfying candidate — and the most expensive: it requires message representations beyond
scalar SP warnings (reweighted surveys / forcing-bit models). Exact small-\(N\) enumeration
of biased measures (\(b^{V_1}t^{C_1}\) over critical clauses/supporting variables) supports
feasibility in principle, with high effective sample sizes and sign-consistent core
reduction. The \(m\)-interpolation parameter already present in SP↔BP message normalizations
is a possible cheap route, pending analytic verification of the associated free-entropy
formula (naive use gives inconsistent readouts).

**Working hypothesis.** (a) is the null/upper-bound candidate at one-step horizon; (b) is
the cheap microscopic alternative testable immediately; (c) is the favored fundamental
candidate, deferred until message representations are verified. The program compares them
first as one-step trial energies, then as rollout costs at horizon \(H>1\), and finally —
if depth matters — under genuine Gibbs (\(M>1\), \(\lambda\)-sweep) sampling.

## VI. From theory to experiment

The staged program (3-SAT and 4-SAT near threshold, \(N=300\)–\(10000\)) is:

- **E0 — baselines.** Reproduce BSP/SID reference curves \(p_{\mathrm{succ}}(\alpha)\)
  and \(\Sigma(t)\) trajectories for certainty, polarization, softened
  (\(\gamma\)) and dynamic-\(I\) policies.
- **E1 — collapse signatures.** Measure, on fixed-point traces, which observables
  (descent rate \(d\Sigma/dt\), \(\eta\), contraction, low-margin fraction) predict
  eventual failure, at which lead time (truncated-trajectory AUC). *Prediction of the
  framework:* stability observables lead, \(\Sigma\)-level observables follow.
- **E2 — one-step \(\Sigma\) oracle.** Exact lookahead (fork, fix, reconverge, rank by
  residual \(\Sigma\)): the upper bound of energy (a) at horizon 1. *Falsifier:* no
  \(p_{\mathrm{succ}}\) gain (or harm) kills (a) at \(H=1\).
- **E3 — Boltzmann on existing scores.** Softmax sampling \(\propto e^{\mathrm{score}/T}\)
  over shortlists, \(T\)-sweep with \(T\to 0\) (greedy) and \(T\to\infty\) (random)
  controls. *Falsifier:* no gain isolates the problem to the *score*, not the *hardness*.
- **E4a — one-step energy comparison.** Same trials, three rankings (\(\Sigma\)-residual,
  post-fix SP cost, gated hybrid) + per-trial logging for counterfactual agreement
  analysis. *Prediction:* energies disagree often; disagreement is harmless on easy
  instances, decisive on hard ones.
- **E4b — triggered \(H\)-step rollout.** At fragile steps (SP-cost trigger), \(H=2\)–\(3\)
  greedy rollouts per candidate; rank by final \(\Sigma\) or summed SP cost; controls
  (H=1-at-trigger, random-at-trigger, greedy). *Prediction:* if depth matters, H=3 beats
  H=1 at fixed energy on seeds where greedy fails. Kill on zero rescues.
- **E4b2 — Gibbs proper.** Stochastic rollouts (\(M>1\)) with \(\lambda\)-sweep from
  risk-sensitive to mean-cost. Direct test of the Gibbs-trajectory hypothesis; requires
  E4b-positive.
- **E7 — threshold validation.** Winners only: fine \(\alpha\)-grids, \(\geq 50\)–\(100\)
  seeds/cell, \(N\) up to \(5000\)–\(10000\), paired tests, per-budget normalization.

## VII. Preliminary numerical evidence (status)

E1 (\(n=20\) runs/cell): max prefix SP effort predicts failure from 10% of the trajectory
(AUC \(0.92\)–\(1.00\) on 3-SAT, \(0.82\) on 4-SAT); retained-\(\Sigma\) fraction follows
later on 3-SAT but is \(\sim\)chance on 4-SAT — supporting the "incomplete proxy" concern
for (a). E2/E4a (3-SAT, \(N=1000\), \(\alpha=4.15\)): the \(\Sigma\)-oracle *harms*
(greedy-sat \(\to\) WalkSAT-fail at 40\(\times\) cost); SP-cost energy matches greedy
safety; hybrid dies in SP on the same instance the others survive — nonlinear fate
sensitivity. Trial-level sigma-vs-stability agreement is only \(21\)–\(91\%\). E3:
temperature on certainty scores never rescues (3/5 greedy vs 2/5 soft at
\(N=1000,\alpha=4.2\)). E4b: at \(N=80\), H=3-plus-stability rescues a seed where all
H=1 rules fail, while H=3-plus-\(\Sigma\) fails — depth\(\times\)energy factorization;
\(N=1000\) replication in progress. No method has yet improved \(p_{\mathrm{succ}}\) over
reference policies on hard cells: the threshold shift is unobserved to date.

## VIII. Discussion

The emerging picture is that the control *formalism* (soft/Gibbs) is necessary but inert
without the right *energy*: temperature on certainty scores and one-step \(\Sigma\) oracles
both fail, while SP-stability observables carry the earliest fate information and survive
as decision criteria where \(\Sigma\) misleads. Whether this reflects a genuine
thermodynamic advantage of stability-like costs, or merely that \(\eta\) is the cheapest
window into hidden cluster-entropy coordinates, is open — and is exactly why energy (c)
remains the favored long-term candidate: a finite-\(y\) quantity would internalize what
\(\eta\) only correlates with.

Two theoretical questions stand out. First, can a *correct* finite-\(m\) SP free entropy be
implemented at near-SP cost (verified \(m\)-interpolation or compact reweighted messages)?
Second, what sets the required rollout horizon — is there a characteristic "correlation
time" of decimation, in steps, beyond which trajectory ensembles beat one-step energies?
The \(H\)-sweep in E4b is designed to measure it. A positive answer to either would turn
the present diagnostic program into a solver that shifts \(\alpha_a\); a negative answer
on both would itself be informative, localizing the algorithmic barrier outside
near-sighted control.

## References

[1] M. Mézard, G. Parisi, and R. Zecchina, Science **297**, 812 (2002).
[2] M. Mézard and G. Parisi, J. Stat. Phys. **111**, 1 (2003).
[3] M. Mézard, M. Palassini, and O. Rivoire, Phys. Rev. Lett. **95**, 200202 (2005).
[4] J. Ding, A. Sly, and N. Sun, Ann. Math. **196**, 1 (2022).
[5] A. Braunstein and R. Zecchina, J. Stat. Mech. P06007 (2004).
[6] G. Parisi, arXiv:cs/0212009 (2002).
[7] G. Parisi, arXiv:cond-mat/0308510 (2003).
[8] R. Marino, G. Parisi, and F. Ricci-Tersenghi, Nat. Commun. **7**, 12996 (2016).
[9] H. J. Kappen, Phys. Rev. Lett. **95**, 200201 (2005).
[10] H. J. Kappen, J. Stat. Mech. P11011 (2005).
[11] E. Todorov, in *Advances in Neural Information Processing Systems* 19 (2007).
[12] B. D. Ziebart et al., in *Proc. AAAI* (2008).
[13] S. Levine et al., J. Mach. Learn. Res. **18**, 1 (2017).
[14] T. Haarnoja et al., in *Proc. ICML* (2018).
[15] M. Mézard and A. Montanari, *Information, Physics, and Computation* (Oxford, 2009).
[16] F. Krzakala et al., Proc. Natl. Acad. Sci. **104**, 10318 (2007).
