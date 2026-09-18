# Idea 023 — SP transpose Jacobian and adjoint sensitivity

## Goal

Compute the signed stability response of candidate moves. Right-mode energy
`v_e^2` was not predictive; eigenvalue sensitivity requires both dominant
modes:

`delta_lambda_i = w^T delta(J_i) v / (w^T v)`.

## First gate

Implement a matrix-free reverse accumulation for the exact transpose of the
validated synchronous SP Jacobian. It reverses:

- clause numerator and normalization products with prefix/suffix products;
- `S`, `U`, warning and normalization factors;
- cavity divisors;
- vertex sign-products;
- damping and ZERO-clamp branches.

The implementation is accepted only if random active vectors satisfy

`<u, Jv> = <J^T u, v>`

to floating-point precision. This check complements the existing central
finite-difference validation of `Jv`.

Only after passing this identity will the experiment compute left/right modes
and test the frozen-topology approximation

`w^T (J_candidate - J) v / (w^T v)`

against the exact post-fix Lyapunov dataset at the normal and fatal
checkpoints.
