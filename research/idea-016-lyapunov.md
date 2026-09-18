# Idea 016 — Lyapunov stability of the SP message fixed point

## Question

Does the plateau-to-collapse transition in complexity coincide with loss of
local stability of the SP message fixed point? Do certainty and polarization
select variables whose fixation preserves that stability?

The messages, not the variable surveys, solve the fixed-point equation

`eta = F(eta)`.

For the implemented complete message sweep, perturbations obey

`delta_eta_(t+1) = J(eta*) delta_eta_t`.

The largest Lyapunov exponent per sweep is

`lambda_max = log(rho(J))`.

Negative lambda means local attraction, lambda approaching zero means critical
slowing, and positive lambda means exponential perturbation growth. This
iteration stability is distinct from both the sign of complexity and the
physical type-II/bug-proliferation stability of the 1RSB solution.

## Implementation

`--lyapunov` adds a read-only analytic Jacobian-vector product for the exact
implemented SP sweep. Cached vertex products are refreshed only after every
clause has been updated, so the sweep is a synchronous/Jacobi map. The tangent
calculation differentiates vertex products, cavity divisors, clause warning
numerators and normalizations, ZERO clamping, and message damping.

A deterministic Benettin/power iteration estimates the dominant tangent
growth without constructing the Jacobian. The diagnostic records
`lyapunov_rho`, `lyapunov_exponent`, and tangent iterations in the normal step
CSV. Because the production tolerance `epsilon=0.01` stops at an approximate
fixed point, it also forks the graph, refines the same state to `ZERO`, and
records a separate tight exponent and complexity. The refined child is
discarded. The diagnostic requires `--diag` and does not change parent
messages, surveys, complexity, or the move schedule.

`--lyapunov-check` independently evaluates the same sweep on
`eta +/- h v` for four central-difference scales from `1e-4` to `1e-7` and
records the minimum relative error against analytic `Jv`.

## Falsifiable first experiment

1. Prove baseline runs with and without the diagnostic have identical numeric
   Sigma trajectories and outcomes.
2. On known N=300 fatal seeds, compare lambda at the final pre-failure fixed
   point with its within-run distribution.
3. Check directly that converged states with negative complexity can still
   have negative lambda.
4. Only if lambda predicts failure, use post-fix lambda as a stability
   constraint: among moves retaining an attractive fixed point, maximize
   residual Sigma.

Kill this direction if the analytic tangent fails finite-difference checks, if
the read-only diagnostic changes trajectories, or if lambda carries no
information beyond ordinary convergence iterations.
