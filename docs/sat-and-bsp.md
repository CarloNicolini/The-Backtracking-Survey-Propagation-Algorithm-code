# SAT, survey propagation and backtracking

BSP searches for assignments that satisfy every clause of a Boolean formula.
Its messages describe the constraints on variables, and its backtracking moves
allow those constraints to be reconsidered as the formula is simplified.

## Formulas and clause density

Let $x_i\in\{0,1\}$ denote the value of variable $i$. A literal is either $x_i$
or its negation $1-x_i$. A clause is the disjunction of its literals, and a
formula in conjunctive normal form (CNF) is the conjunction of its clauses:

$$
C_a(x)=\bigvee_{i\in\partial a}\ell_{ai}(x_i),
\qquad F(x)=\bigwedge_{a=1}^{M}C_a(x).
$$

Here $\partial a$ lists the variables in clause $a$. The SAT problem asks whether
there is an assignment with $F(x)=1$. In $K$-SAT every clause contains $K$
literals. A useful energy counts the violated clauses,

$$
E(x)=\sum_{a=1}^{M}\prod_{i\in\partial a}[1-\ell_{ai}(x_i)].
$$

A satisfying assignment has $E(x)=0$. Each term vanishes as soon as one literal
in its clause is true. In random $K$-SAT, the clause density $\alpha=M/N$ measures
how many constraints are imposed on $N$ variables. For a fixed $N$, adding random
clauses typically reduces the set of solutions and changes its organization.
Solver success is therefore studied as a function of $\alpha$.

For example, consider

$$
F=(x_1\vee\neg x_2\vee x_3)
\wedge(\neg x_1\vee x_2\vee x_4)
\wedge(\neg x_2\vee\neg x_3\vee\neg x_4).
$$

The assignment $(1,0,1,1)$ satisfies all three clauses. Its Python representation
uses positive literals for true variables and negative literals for false ones:

```python
import pybsp

formula = pybsp.Formula([[1, -2, 3], [-1, 2, 4], [-2, -3, -4]], n=4)
assert formula.is_satisfied([1, -2, 3, 4])
```

## The factor graph

The factor graph has one node for each variable and one for each clause. An edge
connects $i$ to $a$ whenever $i\in\partial a$, carrying the sign of that literal.
The same variable participates in several clauses, so choosing its value changes
several constraints at once. Hover over the example graph to inspect the incidences.

```{raw} html
<iframe src="_static/figures/factor_graph.html" title="Signed factor graph of the example CNF" width="100%" height="480" style="border:0" loading="lazy"></iframe>
```

Messages travel along these edges. A cavity message from a variable to a clause
uses the other neighboring clauses, excluding its recipient. This exclusion
avoids feeding a clause's own constraint directly back into its incoming message.

## Survey propagation

In the cavity description, solutions may form clusters with different frozen
variables. SP estimates how often a variable is forced true, forced false, or
remains free across these clusters. These three components appear in
`result.surveys` as `sT`, `sF` and `sI`. They describe an approximate cluster
measure. A free component allows both values within that description.
The underlying construction is developed in
[Braunstein, Mézard and Zecchina's survey propagation paper](https://arxiv.org/abs/cs/0212002).

For plain zero-temperature SP, let $\eta_{a\to i}$ be the survey of a warning
from clause $a$ to variable $i$. Let $\partial_i^+$ and $\partial_i^-$ denote
clauses in which $i$ occurs positively and negatively. Define cavity products

$$
P^+_{i\to a}=\prod_{b\in\partial_i^+\setminus\{a\}}(1-\eta_{b\to i}),
\qquad
P^-_{i\to a}=\prod_{b\in\partial_i^-\setminus\{a\}}(1-\eta_{b\to i}).
$$

Under the cavity independence assumption, a positive normalization gives

$$
s^T_{i\to a}=\frac{(1-P^+_{i\to a})P^-_{i\to a}}{Z_{i\to a}},\quad
s^F_{i\to a}=\frac{(1-P^-_{i\to a})P^+_{i\to a}}{Z_{i\to a}},\quad
s^I_{i\to a}=\frac{P^+_{i\to a}P^-_{i\to a}}{Z_{i\to a}},
$$

where $Z_{i\to a}=P^+_{i\to a}+P^-_{i\to a}-P^+_{i\to a}P^-_{i\to a}$
excludes contradictory warnings. A clause sends a warning when its other
variables are all forced to violate their literals:

$$
\eta_{a\to i}=\prod_{j\in\partial a\setminus\{i\}}q_{j\to a},\qquad
q_{j\to a}=\begin{cases}
s^F_{j\to a},&\ell_{aj}=x_j,\\
s^T_{j\to a},&\ell_{aj}=\neg x_j.
\end{cases}
$$

Iterating these equations seeks a fixed point. Including all incoming clauses
in the products gives the full-variable surveys used for assignment decisions.
On a finite graph with loops, convergence and the quality of the cavity
approximation require empirical checks.
The corresponding cluster-complexity estimate, $\Sigma\simeq\log\mathcal N_{\rm cl}$,
is recorded in `steps["Sigma"]`. It tracks the approximate number of clusters
remaining during simplification.

## From SP to BSP

Survey-inspired decimation assigns variables using the converged surveys and
simplifies the remaining clauses. The default scorer favors high certitude,
$1-\min(s^T_i,s^F_i)$, and the preferred direction follows the larger forced
component. Unit propagation fixes variables forced by the residual clauses.

BSP interleaves these assignments with releases of earlier choices whose
support has weakened. The modified residual formula is followed by another
SP convergence. The baseline parameter $0\le r<1$ controls the target ratio of
backtracking moves to decimation moves, with $r=0$ giving decimation alone.
This revision mechanism is described by
[Marino, Parisi and Ricci-Tersenghi](https://arxiv.org/abs/1508.05117).

In this implementation, SP guides simplification and WalkSAT searches the
remaining formula. Returned assignments are checked against the original CNF.
`steps` records the SP fixed points and `moves` records the assignment/release
trajectory. Failure can arise from a contradiction along that trajectory or
from unsuccessful convergence or search. It does not certify that the original
formula has no satisfying assignment.

```python
result = pybsp.solve(formula, pybsp.SolverOptions(r=0.9, seed=1))
result.steps.head()
result.moves.head()
```

The equations above describe the default SP closure. Thermodynamic and two-step
options introduce additional assumptions and observables, documented in the
[detailed guide](python-native.md#options). The small example illustrates the
message structure. Performance comparisons need ensembles of independent
formulas, as shown in the [experiment tutorial](experiments.md).
