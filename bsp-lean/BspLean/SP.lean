import Mathlib.Algebra.BigOperators.Ring.Finset
import Mathlib.Algebra.Order.BigOperators.GroupWithZero.Finset
import Mathlib.Basic.Real.Basic
import Mathlib.Tactic.Linarith
import Mathlib.Tactic.Ring

/-!
# Survey propagation factors of one cavity variable

For a clause `a` and a variable `j ∈ ∂a`, `A` is the product of `1 - η` over the clauses
where `j` has the same sign as in `a` (`_prod_S` in `src/Graph.cpp`), and `B` is the same
product over the clauses with the opposite sign (`_prod_U`).
-/

namespace BspLean

open Finset

/-- `j` is forced to violate the target clause: a warning from `U` and none from `S`. -/
def piU (A B : ℝ) : ℝ := (1 - B) * A

/-- `j` is forced to satisfy the target clause. -/
def piS (A B : ℝ) : ℝ := (1 - A) * B

/-- `j` receives no warning. -/
def pi0 (A B : ℝ) : ℝ := A * B

/-- No contradiction at `j`: the SP normalization. -/
def normSP (A B : ℝ) : ℝ := A + B - A * B

/-- The SP factor of `j` in the message of the target clause. -/
noncomputable def rho (A B : ℝ) : ℝ := piU A B / normSP A B

theorem pi_sum (A B : ℝ) : piU A B + piS A B + pi0 A B = normSP A B := by
  unfold piU piS pi0 normSP; ring

theorem normSP_eq_one_sub (A B : ℝ) : normSP A B = 1 - (1 - A) * (1 - B) := by
  unfold normSP; ring

section Bounds

variable {A B : ℝ}

theorem piU_nonneg (hA : 0 ≤ A) (hB1 : B ≤ 1) : 0 ≤ piU A B :=
  mul_nonneg (by linarith) hA

theorem normSP_nonneg (hA : 0 ≤ A) (hB : 0 ≤ B) (hB1 : B ≤ 1) : 0 ≤ normSP A B := by
  unfold normSP; nlinarith [mul_nonneg hA (sub_nonneg.2 hB1)]

theorem normSP_le_one (hA1 : A ≤ 1) (hB1 : B ≤ 1) : normSP A B ≤ 1 := by
  rw [normSP_eq_one_sub]; nlinarith [mul_nonneg (sub_nonneg.2 hA1) (sub_nonneg.2 hB1)]

theorem piU_le_normSP (hB : 0 ≤ B) : piU A B ≤ normSP A B := by
  unfold piU normSP; nlinarith

theorem rho_mem_Icc (hA : 0 ≤ A) (hB : 0 ≤ B) (hB1 : B ≤ 1) : rho A B ∈ Set.Icc 0 1 :=
  ⟨div_nonneg (piU_nonneg hA hB1) (normSP_nonneg hA hB hB1),
    div_le_one_of_le₀ (piU_le_normSP hB) (normSP_nonneg hA hB hB1)⟩

/-- `N ρ = π^u` also when `N = 0`, because then `π^u = 0`. -/
theorem normSP_mul_rho (hA : 0 ≤ A) (hB : 0 ≤ B) (hB1 : B ≤ 1) :
    normSP A B * rho A B = piU A B := by
  rcases eq_or_ne (normSP A B) 0 with h | h
  · have h1 := piU_le_normSP (A := A) hB
    rw [h] at h1
    rw [rho, h, zero_mul]
    linarith [piU_nonneg hA hB1]
  · rw [rho, mul_div_cancel₀ _ h]

end Bounds

theorem prod_one_sub_mem_Icc {ι : Type*} (s : Finset ι) (η : ι → ℝ)
    (h : ∀ b ∈ s, η b ∈ Set.Icc 0 1) : ∏ b ∈ s, (1 - η b) ∈ Set.Icc 0 1 :=
  ⟨prod_nonneg fun b hb => by linarith [(h b hb).2],
    prod_le_one₀ (fun b hb => by linarith [(h b hb).2]) fun b hb => by linarith [(h b hb).1]⟩

/-- The SP message of the target clause: the product of the factors of its other variables. -/
noncomputable def spMessage {J : Type*} [Fintype J] (A B : J → ℝ) : ℝ :=
  ∏ j, rho (A j) (B j)

theorem spMessage_mem_Icc {J : Type*} [Fintype J] (A B : J → ℝ) (hA : ∀ j, 0 ≤ A j)
    (hB : ∀ j, B j ∈ Set.Icc 0 1) : spMessage A B ∈ Set.Icc 0 1 := by
  have h : ∀ j, rho (A j) (B j) ∈ Set.Icc 0 1 := fun j => rho_mem_Icc (hA j) (hB j).1 (hB j).2
  exact ⟨prod_nonneg fun j _ => (h j).1, prod_le_one₀ (fun j _ => (h j).1) fun j _ => (h j).2⟩

/-! ## The formulas of `src/Graph.cpp` and `include/bsp/Vertex.hpp` -/

/-- `Graph::__pu` with `_prod_S = PS` and `_prod_U = PU`. -/
theorem code_pu (PS PU : ℝ) : (1 - PU) * PS = piU PS PU := rfl

/-- `Graph::__norm` with `rho_SP = 1`. -/
theorem code_norm (PS PU : ℝ) : PS + PU - 1 * PS * PU = normSP PS PU := by
  unfold normSP; ring

/-- `Vertex::_p_PLUS`: the full star of `i`, with `P = prod_V_plus`, `M = prod_V_minus`. -/
theorem code_p_plus (P M : ℝ) : (1 - P) * M = piU M P := rfl

/-- `Vertex::complexity_variable = _p_I + _p_minus + _p_plus`. -/
theorem code_site (P M : ℝ) : P * M + (1 - M) * P + (1 - P) * M = normSP M P := by
  unfold normSP; ring

end BspLean
