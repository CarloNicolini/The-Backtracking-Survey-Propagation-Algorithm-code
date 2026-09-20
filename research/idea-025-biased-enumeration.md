# Idea 025 — exact test of finite-range unfrozen biases

## Rationale

The scalar SP warning contains no internal-entropy information, so inserting a
parameter `y` into the existing update would not implement finite-m 1RSB.
Before building a new message representation, test exact assignment-level
surrogates on fully enumerated small formulas.

For each SAT assignment compute:

- `C1`: clauses with exactly one satisfying literal;
- `V1`: variables supporting at least one such critical clause;
- final whitening-core fraction.

Evaluate the exact reweighted measure

`weight = b^V1 * t^C1`

for the Cartesian response surface

`b in {1, 0.8, 0.6}`, `t in {1, 0.95, 0.9, 0.8}`.

`b=1,t<1` tests the exact two-state critical-clause BP surrogate. A response
only for `b<1` indicates that the four-state forcing-bit model is necessary.
Effective sample size is reported to distinguish a genuine shift from weight
collapse.

The first remote experiment uses random 4-SAT at N=20, alpha=9.7 over
multiple seeds. This is a mechanistic enumeration smoke test, not evidence
about asymptotic algorithmic performance.

## Results

Among 100 formulas, 66 are satisfiable and fully enumerable.

- Two-state critical-clause bias `b=1,t=0.9` lowers mean whitening-core
  fraction by 0.0173 (approximate 95% interval `[-0.0298,-0.0048]`) with mean
  ESS 0.926.
- Supported-variable bias `b=0.8,t=1` lowers the core in 22 formulas, raises
  it in none, and leaves 44 unchanged; mean reduction is 0.0081 with ESS
  0.990.
- `b=0.6,t=0.9` gives mean reduction 0.0284 with ESS 0.880.

The factorized two-state surrogate passes its first gate, but the
supported-variable term is more sign-consistent and supports implementing the
four-state forcing-bit model. These are finite-N mechanistic results and do
not establish an asymptotic BSP improvement.

Artifacts:

- `results/idea-025-k4-n20-a9.7.tsv`
- `results/idea-025-k4-n20-a9.7-seeds21-100.tsv`
