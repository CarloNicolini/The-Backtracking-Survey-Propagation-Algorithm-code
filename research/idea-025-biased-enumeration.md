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
