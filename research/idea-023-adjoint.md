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

## Results

The transpose operator is correct. Relative adjoint-identity errors are
`5.38e-16` initially, `1.05e-16` after `ptrV` reordering, and `5.60e-17` with
damping 0.3.

At the small initial checkpoint, independent left/right power iterations give
eigenvalue 0.89268 versus norm-growth rho 0.89306, with residuals below 0.002.
At the N=300 checkpoint 1400, however, the dominant mode is not a simple real
eigenvalue: `w^T v=0.0103`, left/right residuals are 4.87/5.34, and the
biorthogonal quotient spuriously gives 3.71 while norm growth is 0.961.

Consequently the mask-only candidate sensitivity has Pearson/Spearman
correlations only 0.079/0.082 with exact post-fix rho. Those scores are
invalid and are not used for a policy.

Decision: keep the validated matrix-free `J^T` implementation. Replace scalar
power iteration with Arnoldi/real-Schur treatment of complex or nearly
degenerate dominant subspaces before revisiting `w^T deltaJ_i v`.
