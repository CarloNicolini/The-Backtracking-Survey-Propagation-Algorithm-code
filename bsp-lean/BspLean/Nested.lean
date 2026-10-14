import Mathlib.Algebra.BigOperators.Field
import Mathlib.Analysis.SpecialFunctions.Pow.Continuity
import Mathlib.Analysis.SpecialFunctions.Log.Deriv
import Mathlib.Analysis.SpecialFunctions.ExpDeriv
import Mathlib.Analysis.SpecialFunctions.Log.NegMulLog

/-!
# The nested partition function of 2RSB

`Z2 m1 m2 Z = ∑_α (∑_γ Z_{αγ}^{m2})^{m1/m2}`: `α` runs over the clusters and `γ` over the
sub-clusters of a cluster. The lemmas below are the finite steps of the derivation in
`notes/2rsb_sp_ksat.md`.

* `Z2_mul`: `Z2` factorizes over independent subsystems. This is the cavity step.
* `Z2_same`: with `m1 = m2` the two levels merge into one 1RSB sum.
* `innerCount_tendsto`, `Z2_zeroEnergy_tendsto`: at zero energy with `m1/m2 = x` fixed,
  `Z2 → ∑_α 𝒩_α^x`, where `𝒩_α` counts the zero-energy sub-clusters of `α`.
* `clusterWeight_reweight`: an added piece that multiplies `𝒩_α` by `z_α` reweights the
  clusters by `z_α^x`.
* `hasDerivAt_Phi`, `sigma1_eq_entropy`, `sigma1_nonneg`: for `Φ(x) = log ∑ 𝒩_α^x`,
  `Φ' = ∑ w_α log 𝒩_α` and `Φ - xΦ' = -∑ w_α log w_α ≥ 0`.
-/

namespace BspLean

open Finset Filter Topology

section Z2

variable {α γ : Type*} [Fintype α] [Fintype γ]

/-- The nested (two-level) partition function. -/
noncomputable def Z2 (m1 m2 : ℝ) (Z : α → γ → ℝ) : ℝ :=
  ∑ a, (∑ g, Z a g ^ m2) ^ (m1 / m2)

theorem Z2_mul {α' γ' : Type*} [Fintype α'] [Fintype γ'] (m1 m2 : ℝ) (Z : α → γ → ℝ)
    (Z' : α' → γ' → ℝ) (hZ : ∀ a g, 0 ≤ Z a g) (hZ' : ∀ a g, 0 ≤ Z' a g) :
    Z2 m1 m2 (fun (p : α × α') (r : γ × γ') => Z p.1 r.1 * Z' p.2 r.2) =
      Z2 m1 m2 Z * Z2 m1 m2 Z' := by
  have inner : ∀ a a', ∑ r : γ × γ', (Z a r.1 * Z' a' r.2) ^ m2 =
      (∑ g, Z a g ^ m2) * ∑ g, Z' a' g ^ m2 := fun a a' => by
    rw [Fintype.sum_prod_type, sum_mul_sum]
    exact sum_congr rfl fun g _ => sum_congr rfl fun g' _ => Real.mul_rpow (hZ a g) (hZ' a' g')
  rw [Z2, Z2, Z2, Fintype.sum_prod_type, sum_mul_sum]
  refine sum_congr rfl fun a _ => sum_congr rfl fun a' _ => ?_
  rw [inner, Real.mul_rpow (sum_nonneg fun g _ => Real.rpow_nonneg (hZ a g) _)
    (sum_nonneg fun g _ => Real.rpow_nonneg (hZ' a' g) _)]

theorem Z2_same (m : ℝ) (hm : m ≠ 0) (Z : α → γ → ℝ) : Z2 m m Z = ∑ a, ∑ g, Z a g ^ m := by
  simp [Z2, div_self hm]

/-- Boltzmann weights at inverse temperature `β`: `Z^{m2} = exp(-μ2 E)` with `μ2 = β m2`. -/
theorem Z2_boltzmann (x m2 β : ℝ) (hm2 : m2 ≠ 0) (E : α → γ → ℝ) :
    Z2 (x * m2) m2 (fun a g => Real.exp (-(β * E a g))) =
      ∑ a, (∑ g, Real.exp (-((β * m2) * E a g))) ^ x := by
  rw [Z2, mul_div_cancel_right₀ x hm2]
  refine sum_congr rfl fun a _ => congrArg (· ^ x) (sum_congr rfl fun g _ => ?_)
  rw [← Real.exp_mul]; ring_nf

theorem innerCount_tendsto (E : γ → ℝ) (hE : ∀ g, 0 ≤ E g) :
    Tendsto (fun μ => ∑ g, Real.exp (-(μ * E g))) atTop (𝓝 (#{g | E g = 0} : ℝ)) := by
  have h : ∀ g, Tendsto (fun μ => Real.exp (-(μ * E g))) atTop
      (𝓝 (if E g = 0 then 1 else 0)) := fun g => by
    split_ifs with h0
    · simp [h0]
    · exact Real.tendsto_exp_neg_atTop_nhds_zero.comp
        (tendsto_id.atTop_mul_const (lt_of_le_of_ne (hE g) (Ne.symm h0)))
  simpa [sum_boole] using tendsto_finsetSum univ fun g _ => h g

theorem Z2_zeroEnergy_tendsto (x : ℝ) (hx : 0 < x) (E : α → γ → ℝ) (hE : ∀ a g, 0 ≤ E a g) :
    Tendsto (fun μ => ∑ a, (∑ g, Real.exp (-(μ * E a g))) ^ x) atTop
      (𝓝 (∑ a, (#{g | E a g = 0} : ℝ) ^ x)) :=
  tendsto_finsetSum univ fun a _ =>
    (innerCount_tendsto (E a) (hE a)).rpow_const (Or.inr hx.le)

end Z2

section Weights

variable {α : Type*} [Fintype α]

theorem clusterWeight_reweight (N z : α → ℝ) (hN : ∀ a, 0 ≤ N a) (hz : ∀ a, 0 ≤ z a) (x : ℝ)
    (hS : ∑ b, N b ^ x ≠ 0) (a : α) :
    (N a * z a) ^ x / ∑ b, (N b * z b) ^ x =
      (N a ^ x / ∑ b, N b ^ x) * z a ^ x / ∑ b, (N b ^ x / ∑ c, N c ^ x) * z b ^ x := by
  simp_rw [Real.mul_rpow (hN _) (hz _), div_mul_eq_mul_div, ← sum_div]
  rw [div_div_div_cancel_right₀ hS]

/-- `Φ(x) = log ∑_α exp(x ℓ_α)`, with `ℓ_α = log 𝒩_α`. -/
noncomputable def Phi (ℓ : α → ℝ) (x : ℝ) : ℝ := Real.log (∑ a, Real.exp (x * ℓ a))

/-- The weight of the cluster `a`, proportional to `𝒩_a^x`. -/
noncomputable def clusterW (ℓ : α → ℝ) (x : ℝ) (a : α) : ℝ :=
  Real.exp (x * ℓ a) / ∑ b, Real.exp (x * ℓ b)

theorem Phi_rpow (N : α → ℝ) (hN : ∀ a, 0 < N a) (x : ℝ) :
    Phi (fun a => Real.log (N a)) x = Real.log (∑ a, N a ^ x) := by
  simp_rw [Phi, Real.rpow_def_of_pos (hN _), mul_comm x]

variable [Nonempty α] (ℓ : α → ℝ) (x : ℝ)

theorem sum_exp_pos : 0 < ∑ a, Real.exp (x * ℓ a) :=
  sum_pos (fun _ _ => Real.exp_pos _) univ_nonempty

theorem sum_clusterW : ∑ a, clusterW ℓ x a = 1 := by
  simp only [clusterW]
  rw [← sum_div, div_self (sum_exp_pos ℓ x).ne']

theorem hasDerivAt_Phi : HasDerivAt (Phi ℓ) (∑ a, clusterW ℓ x a * ℓ a) x := by
  have h : HasDerivAt (fun y => ∑ a, Real.exp (y * ℓ a)) (∑ a, Real.exp (x * ℓ a) * ℓ a) x :=
    HasDerivAt.fun_sum fun a _ => by
      have h := ((hasDerivAt_id' x).mul_const (ℓ a)).exp
      rwa [one_mul] at h
  rw [show ∑ a, clusterW ℓ x a * ℓ a = (∑ a, Real.exp (x * ℓ a) * ℓ a) /
      ∑ a, Real.exp (x * ℓ a) by simp_rw [clusterW, div_mul_eq_mul_div, ← sum_div]]
  exact h.log (sum_exp_pos ℓ x).ne'

/-- The cluster complexity `Σ_1 = Φ - xΦ'` is the entropy of the cluster weights. -/
theorem sigma1_eq_entropy :
    Phi ℓ x - x * ∑ a, clusterW ℓ x a * ℓ a = ∑ a, Real.negMulLog (clusterW ℓ x a) := by
  have hlog : ∀ a, Real.log (clusterW ℓ x a) = x * ℓ a - Phi ℓ x := fun a => by
    rw [clusterW, Real.log_div (Real.exp_pos _).ne' (sum_exp_pos ℓ x).ne', Real.log_exp, Phi]
  have h : ∑ a, -clusterW ℓ x a * (x * ℓ a - Phi ℓ x) =
      Phi ℓ x * ∑ a, clusterW ℓ x a - x * ∑ a, clusterW ℓ x a * ℓ a := by
    rw [mul_sum, mul_sum, ← sum_sub_distrib]
    exact sum_congr rfl fun a _ => by ring
  simp_rw [Real.negMulLog, hlog]
  rw [h, sum_clusterW, mul_one]

theorem sigma1_nonneg : 0 ≤ Phi ℓ x - x * ∑ a, clusterW ℓ x a * ℓ a := by
  rw [sigma1_eq_entropy]
  refine sum_nonneg fun a _ => Real.negMulLog_nonneg (div_nonneg (Real.exp_pos _).le
    (sum_exp_pos ℓ x).le) (div_le_one_of_le₀ ?_ (sum_exp_pos ℓ x).le)
  exact single_le_sum (fun b _ => (Real.exp_pos (x * ℓ b)).le) (mem_univ a)

end Weights

end BspLean
