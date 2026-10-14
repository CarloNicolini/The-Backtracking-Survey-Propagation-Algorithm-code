import BspLean.ThermoLinear
import Mathlib.LinearAlgebra.Matrix.Charpoly.Basic

/-!
# Gauge and zero-energy limit of the stability operator at finite `μ`

The widths of a clause message live near its two corners. The outer weight of a corner
configuration of energy `E` is `e^{-μ₁ E}` (`R2y_mean`), and the slopes of `ThermoLinear`
give the raw coefficients per unit of input width, for the other warnings at counts `k`:
`e^{-μ₁ k}` in the channel U (same corner), `e^{-μ₁ k} e^{-μ₂}` in the channel S from the
corner 0 to the corner 1, and `e^{-μ₁ (k+1)} e^{μ₂}` from the corner 1 to the corner 0.

* `gauge01`, `gauge10`, `cycle_S`: with the widths near the corner 1 scaled by `e^{μ₂}`, the
  S coefficients lose `μ₂`. The product around the 2-cycle has no `μ₂` in any scale.
* `charpoly_gauge`: `fromBlocks U (e^{μ₂} • S₁₀) (e^{-μ₂} • S₀₁) U` and
  `fromBlocks U S₁₀ S₀₁ U` have the same characteristic polynomial for every `μ₂`, so the
  growth rate `λ` depends on `μ₁` only (MPRT, appendix A).
* `expSum_tendsto_zero`, `expSum_tendsto_head`: as `μ₁ → ∞` the S coefficients from the
  corner 1 vanish, and the U coefficients keep only the conflict-free term `k = 0`, which is
  the coefficient `A B' / N` of the zero-energy test (`T_coefficient`, `thermoZ_tendsto`).
* `limit_charpoly`: the limit operator is block triangular with U twice on the diagonal,
  so its eigenvalues are those of U. The channel S from the corner 0 to the corner 1 keeps
  an `O(1)` coefficient but does not change `λ` in the zero-energy limit.
* `charpoly_symmetric`: with the scale `e^{μ₂ - μ₁/2}` both S coefficients are
  `e^{-μ₁ (k + 1/2)}` and the operator splits into `U + S` and `U - S`. This is the one width
  per edge of `rsb2_lambda_y`.
* `abs_pow_sub_le`: for nonnegative `U` and `S` the powers of `U - S` are bounded entrywise by
  those of `U + S`, so `U + S` carries the growth rate.
-/

namespace BspLean

open Finset Filter Topology Matrix

section Coefficients

variable (μ₁ μ₂ : ℝ) (k : ℕ)

theorem gauge01 :
    Real.exp μ₂ * (Real.exp (-(μ₁ * k)) * Real.exp (-μ₂)) = Real.exp (-(μ₁ * k)) := by
  rw [mul_left_comm, ← Real.exp_add, add_neg_cancel, Real.exp_zero, mul_one]

theorem gauge10 :
    Real.exp (-μ₂) * (Real.exp (-(μ₁ * (k + 1))) * Real.exp μ₂) =
      Real.exp (-(μ₁ * (k + 1))) := by
  rw [mul_left_comm, ← Real.exp_add, neg_add_cancel, Real.exp_zero, mul_one]

theorem cycle_S :
    (Real.exp (-(μ₁ * k)) * Real.exp (-μ₂)) * (Real.exp (-(μ₁ * (k + 1))) * Real.exp μ₂) =
      Real.exp (-(μ₁ * (2 * k + 1))) := by
  simp only [← Real.exp_add]
  ring_nf

end Coefficients

section Operator

variable {n : Type*} [Fintype n] [DecidableEq n]

/-- Rows and columns: the widths near the corner 0, then those near the corner 1. -/
theorem charpoly_gauge (U S₁₀ S₀₁ : Matrix n n ℝ) (μ₂ : ℝ) :
    (fromBlocks U (Real.exp μ₂ • S₁₀) (Real.exp (-μ₂) • S₀₁) U).charpoly =
      (fromBlocks U S₁₀ S₀₁ U).charpoly := by
  set T := fromBlocks U (Real.exp μ₂ • S₁₀) (Real.exp (-μ₂) • S₀₁) U
  have hc : Real.exp (-μ₂) * Real.exp μ₂ = 1 := by rw [← Real.exp_add]; simp
  let D : Matrix (n ⊕ n) (n ⊕ n) ℝ := fromBlocks 1 0 0 (Real.exp μ₂ • 1)
  let Di : Matrix (n ⊕ n) (n ⊕ n) ℝ := fromBlocks 1 0 0 (Real.exp (-μ₂) • 1)
  have hDi : Di * D = 1 := by
    simp only [D, Di, fromBlocks_multiply, smul_mul_smul_comm, hc, one_smul, Matrix.one_mul,
      Matrix.mul_zero, Matrix.zero_mul, add_zero, zero_add, fromBlocks_one]
  have hconj : fromBlocks U S₁₀ S₀₁ U = D * T * Di := by
    simp only [T, D, Di, fromBlocks_multiply, Matrix.one_mul, Matrix.mul_one, Matrix.mul_zero,
      Matrix.zero_mul, add_zero, zero_add, Matrix.smul_mul, Matrix.mul_smul, smul_smul, smul_zero,
      hc, one_smul]
  rw [hconj, charpoly_mul_comm, ← Matrix.mul_assoc, hDi, Matrix.one_mul]

theorem limit_charpoly (U S₀₁ : Matrix n n ℝ) :
    (fromBlocks U 0 S₀₁ U).charpoly = U.charpoly ^ 2 := by
  rw [charpoly_fromBlocks_zero₁₂, sq]

/-- With the scale `e^{μ₂ - μ₁/2}` both S blocks are equal, and the operator splits into the
sum and the difference of the two corners. For nonnegative `U` and `S` the spectral radius is
that of `U + S`: one width per edge. -/
theorem charpoly_symmetric (U S : Matrix n n ℝ) :
    (fromBlocks U S S U).charpoly = (U + S).charpoly * (U - S).charpoly := by
  let P : Matrix (n ⊕ n) (n ⊕ n) ℝ := fromBlocks 1 1 1 (-1)
  let Pi : Matrix (n ⊕ n) (n ⊕ n) ℝ := (2 : ℝ)⁻¹ • P
  have hP : P * Pi = 1 := by
    simp only [Pi, P, Matrix.mul_smul, fromBlocks_multiply, Matrix.mul_one, Matrix.mul_neg,
      neg_neg, fromBlocks_smul]
    rw [← fromBlocks_one]
    congr 1 <;> module
  have hconj : fromBlocks U S S U = Pi * (fromBlocks (U + S) 0 0 (U - S) * P) := by
    simp only [Pi, P, Matrix.smul_mul, fromBlocks_multiply, Matrix.one_mul, Matrix.mul_one,
      Matrix.neg_mul, Matrix.mul_neg, neg_zero, add_zero, zero_add, fromBlocks_smul]
    congr 1 <;> module
  rw [hconj, charpoly_mul_comm, Matrix.mul_assoc, hP, Matrix.mul_one,
    charpoly_fromBlocks_zero₁₂]

/-- The powers of `U - S` are bounded entrywise by those of `U + S`, so by Gelfand's formula
`ρ(U - S) ≤ ρ(U + S)`: the growth rate is that of the sum of the two corners. -/
theorem abs_pow_sub_le (U S : Matrix n n ℝ) (hU : ∀ i j, 0 ≤ U i j) (hS : ∀ i j, 0 ≤ S i j)
    (m : ℕ) (i j : n) : |((U - S) ^ m) i j| ≤ ((U + S) ^ m) i j := by
  induction m generalizing j with
  | zero => by_cases h : i = j <;> simp [h]
  | succ m ih =>
    rw [pow_succ, pow_succ, Matrix.mul_apply, Matrix.mul_apply]
    refine (abs_sum_le_sum_abs _ _).trans (sum_le_sum fun l _ => ?_)
    have hl : |(U - S) l j| ≤ (U + S) l j := by
      rw [Matrix.sub_apply, Matrix.add_apply]
      exact abs_le.mpr ⟨by linarith [hU l j, hS l j], by linarith [hU l j, hS l j]⟩
    rw [abs_mul]
    exact mul_le_mul (ih l) hl (abs_nonneg _) ((abs_nonneg _).trans (ih l))

end Operator

section Limits

variable (c : ℕ → ℝ) (m : ℕ)

theorem expSum_tendsto_zero :
    Tendsto (fun μ₁ => ∑ k ∈ range m, c k * Real.exp (-(μ₁ * (k + 1)))) atTop (𝓝 0) := by
  have h : ∀ k : ℕ, Tendsto (fun μ₁ => c k * Real.exp (-(μ₁ * (k + 1)))) atTop (𝓝 0) :=
    fun k => by
      simpa using (Real.tendsto_exp_neg_atTop_nhds_zero.comp
        (tendsto_id.atTop_mul_const (by positivity : (0 : ℝ) < k + 1))).const_mul (c k)
  simpa using tendsto_finsetSum (range m) fun k _ => h k

theorem expSum_tendsto_head :
    Tendsto (fun μ₁ => ∑ k ∈ range (m + 1), c k * Real.exp (-(μ₁ * k))) atTop (𝓝 (c 0)) := by
  have h := (expSum_tendsto_zero (fun k => c (k + 1)) m).add_const (c 0)
  simp only [zero_add] at h
  refine h.congr fun μ₁ => ?_
  rw [sum_range_succ']
  push_cast
  simp

end Limits

end BspLean
