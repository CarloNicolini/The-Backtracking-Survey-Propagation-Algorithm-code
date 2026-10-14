import BspLean.Counting
import Mathlib.Analysis.SpecialFunctions.Pow.Real

/-!
# The zero-energy 2RSB update on finite weighted populations

Each incoming edge `b → j` of the target clause carries a finite population: weights
`q j b` and values `v j b` (the warning probability `η` of one cluster). A cluster sample
`σ` picks one member per incoming edge. Its output value is the SP message
`∏_j ρ_j(σ)` and its weight is `(∏_b q) · (∏_j N_j(σ))^x`. By `noContradiction_prob`,
`∏_j N_j(σ)` is the fraction of the combined sub-states without contradiction, and by
`clusterWeight_reweight` the clusters are reweighted by its power `x`.

* `R1_mean`: populations that are single atoms give the SP message, for every `x`.
* `R2_support`, `R2_mean`: 0/1 populations (the MPRT corner embedding) stay 0/1, and their
  mean is the SP message of the input means, for every `x ≠ 0`.
* `R3_mean`, `R3_marginal`, `Za_mean`: at `x = 1` the means satisfy SP, the marginals are
  the SP marginals of the means, and the clause term of `Φ(1)` is the SP one.
-/

namespace BspLean

open Finset

theorem prod_bit {ι : Type*} (s : Finset ι) (f : ι → ℝ) (h : ∀ i ∈ s, f i = 0 ∨ f i = 1) :
    ∏ i ∈ s, f i = 0 ∨ ∏ i ∈ s, f i = 1 :=
  prod_induction f (fun y => y = 0 ∨ y = 1)
    (fun a b ha hb => by rcases ha with rfl | rfl <;> rcases hb with rfl | rfl <;> simp)
    (Or.inr rfl) h

theorem bit_rpow {y : ℝ} (x : ℝ) (hx : x ≠ 0) (hy : y = 0 ∨ y = 1) : y ^ x = y := by
  rcases hy with rfl | rfl
  exacts [Real.zero_rpow hx, Real.one_rpow x]

section Star

variable {J : Type*} [Fintype J] [DecidableEq J] {κ : J → Type*} [∀ j, Fintype (κ j)]
  [∀ j, DecidableEq (κ j)] {Ω : ∀ j, κ j → Type*} [∀ j b, Fintype (Ω j b)]
  (side : ∀ j, κ j → Bool) (q v : ∀ j b, Ω j b → ℝ)

/-- Weight of the sample `τ` of the incoming edges of `j`. -/
def blockW (j : J) (τ : ∀ b, Ω j b) : ℝ := ∏ b, q j b (τ b)

/-- `A_j` (`t = true`) or `B_j` (`t = false`) of the sample `τ`. -/
def sideAt (t : Bool) (j : J) (τ : ∀ b, Ω j b) : ℝ := sideProd (side j) t (pick (v j) τ)

def piUAt (j : J) (τ : ∀ b, Ω j b) : ℝ := piU (sideAt side v true j τ) (sideAt side v false j τ)

def normAt (j : J) (τ : ∀ b, Ω j b) : ℝ :=
  normSP (sideAt side v true j τ) (sideAt side v false j τ)

noncomputable def rhoAt (j : J) (τ : ∀ b, Ω j b) : ℝ :=
  rho (sideAt side v true j τ) (sideAt side v false j τ)

/-- `A_j` or `B_j` of the population means. -/
noncomputable def sideMean (t : Bool) (j : J) : ℝ := sideProd (side j) t (mean (q j) (v j))

/-- Mean of the output population of the 2RSB update at the Parisi ratio `x`. -/
noncomputable def outMean (x : ℝ) : ℝ :=
  prodExpect (blockW q) (fun σ => (∏ j, normAt side v j (σ j)) ^ x * ∏ j, rhoAt side v j (σ j))
    / prodExpect (blockW q) (fun σ => (∏ j, normAt side v j (σ j)) ^ x)

omit [Fintype J] [DecidableEq J] [∀ j, DecidableEq (κ j)] [∀ j b, Fintype (Ω j b)] in
theorem sideAt_mem_Icc (hv : ∀ j b ξ, v j b ξ ∈ Set.Icc 0 1) (t : Bool) (j : J)
    (τ : ∀ b, Ω j b) : sideAt side v t j τ ∈ Set.Icc 0 1 :=
  prod_one_sub_mem_Icc _ _ fun b _ => hv j b (τ b)

omit [Fintype J] [DecidableEq J] [∀ j, DecidableEq (κ j)] [∀ j b, Fintype (Ω j b)] in
theorem normAt_mul_rhoAt (hv : ∀ j b ξ, v j b ξ ∈ Set.Icc 0 1) (j : J) (τ : ∀ b, Ω j b) :
    normAt side v j τ * rhoAt side v j τ = piUAt side v j τ :=
  normSP_mul_rho (sideAt_mem_Icc side v hv true j τ).1 (sideAt_mem_Icc side v hv false j τ).1
    (sideAt_mem_Icc side v hv false j τ).2

theorem E_prod_piU (hq : ∀ j b, ∑ ξ, q j b ξ = 1) :
    prodExpect (blockW q) (fun σ => ∏ j, piUAt side v j (σ j)) =
      ∏ j, piU (sideMean side q v true j) (sideMean side q v false j) := by
  rw [prodExpect_prod]
  exact prod_congr rfl fun j _ => expect_piU (side j) (q j) (v j) (hq j)

theorem E_prod_normSP (hq : ∀ j b, ∑ ξ, q j b ξ = 1) :
    prodExpect (blockW q) (fun σ => ∏ j, normAt side v j (σ j)) =
      ∏ j, normSP (sideMean side q v true j) (sideMean side q v false j) := by
  rw [prodExpect_prod]
  exact prod_congr rfl fun j _ => expect_normSP (side j) (q j) (v j) (hq j)

/-- R3: at `x = 1` the mean of the output population is the SP message of the means. -/
theorem R3_mean (hq : ∀ j b, ∑ ξ, q j b ξ = 1) (hv : ∀ j b ξ, v j b ξ ∈ Set.Icc 0 1) :
    outMean side q v 1 = spMessage (sideMean side q v true) (sideMean side q v false) := by
  have hnum : ∀ σ : (∀ j b, Ω j b),
      (∏ j, normAt side v j (σ j)) ^ (1 : ℝ) * ∏ j, rhoAt side v j (σ j) =
        ∏ j, piUAt side v j (σ j) := fun σ => by
    rw [Real.rpow_one, ← prod_mul_distrib]
    exact prod_congr rfl fun j _ => normAt_mul_rhoAt side v hv j (σ j)
  simp_rw [outMean, hnum, Real.rpow_one]
  rw [E_prod_piU side q v hq, E_prod_normSP side q v hq, spMessage, ← prod_div_distrib]
  rfl

/-- The clause term of `Φ` at `x = 1`: `E[∏ N_j - ∏ π^u_j]` is its value at the means. -/
theorem Za_mean (hq : ∀ j b, ∑ ξ, q j b ξ = 1) :
    prodExpect (blockW q)
        (fun σ => ∏ j, normAt side v j (σ j) - ∏ j, piUAt side v j (σ j)) =
      ∏ j, normSP (sideMean side q v true j) (sideMean side q v false j) -
        ∏ j, piU (sideMean side q v true j) (sideMean side q v false j) := by
  rw [prodExpect_sub, E_prod_normSP side q v hq, E_prod_piU side q v hq]

theorem prodExpect_mul_const {I : Type*} [Fintype I] [DecidableEq I] {Ξ : I → Type*}
    [∀ i, Fintype (Ξ i)] (p : ∀ i, Ξ i → ℝ) (F : (∀ i, Ξ i) → ℝ) (c : ℝ) :
    prodExpect p (fun σ => F σ * c) = prodExpect p F * c := by
  simp only [prodExpect, sum_mul, mul_assoc]

/-- R1: if every population is the single value `η`, the output is the SP message of `η`. -/
theorem R1_mean (η : ∀ j, κ j → ℝ) (hv : ∀ j b ξ, v j b ξ = η j b) (x : ℝ)
    (hZ : prodExpect (blockW q) (fun σ => (∏ j, normAt side v j (σ j)) ^ x) ≠ 0) :
    outMean side q v x = spMessage (fun j => sideProd (side j) true (η j))
      (fun j => sideProd (side j) false (η j)) := by
  have hpick : ∀ j (τ : ∀ b, Ω j b), pick (v j) τ = η j := fun j τ => funext fun b => hv j b (τ b)
  simp only [outMean, rhoAt, sideAt, hpick]
  rw [prodExpect_mul_const, mul_div_cancel_left₀ _ hZ]
  rfl

omit [Fintype J] [DecidableEq J] [∀ j, DecidableEq (κ j)] [∀ j b, Fintype (Ω j b)] in
theorem sideAt_bit (hv : ∀ j b ξ, v j b ξ = 0 ∨ v j b ξ = 1) (t : Bool) (j : J)
    (τ : ∀ b, Ω j b) : sideAt side v t j τ = 0 ∨ sideAt side v t j τ = 1 :=
  prod_bit _ _ fun b _ => by rcases hv j b (τ b) with h | h <;> simp [pick, h]

omit [DecidableEq J] [∀ j, DecidableEq (κ j)] [∀ j b, Fintype (Ω j b)] in
/-- R2: the corner embedding stays on the corners. -/
theorem R2_support (hv : ∀ j b ξ, v j b ξ = 0 ∨ v j b ξ = 1) (σ : ∀ j b, Ω j b) :
    ∏ j, rhoAt side v j (σ j) = 0 ∨ ∏ j, rhoAt side v j (σ j) = 1 :=
  prod_bit _ _ fun j _ => by
    rcases sideAt_bit side v hv true j (σ j) with h | h <;>
      rcases sideAt_bit side v hv false j (σ j) with h' | h' <;>
      simp [rhoAt, h, h', rho, piU, normSP]

/-- R2: on the corner embedding the update is SP on the corner masses, for every `x ≠ 0`. -/
theorem R2_mean (hq : ∀ j b, ∑ ξ, q j b ξ = 1) (hv : ∀ j b ξ, v j b ξ = 0 ∨ v j b ξ = 1)
    (x : ℝ) (hx : x ≠ 0) :
    outMean side q v x = spMessage (sideMean side q v true) (sideMean side q v false) := by
  have hN : ∀ σ : (∀ j b, Ω j b), (∏ j, normAt side v j (σ j)) ^ x =
      (∏ j, normAt side v j (σ j)) ^ (1 : ℝ) := fun σ => by
    rw [Real.rpow_one]
    refine bit_rpow x hx (prod_bit _ _ fun j _ => ?_)
    rcases sideAt_bit side v hv true j (σ j) with h | h <;>
      rcases sideAt_bit side v hv false j (σ j) with h' | h' <;> simp [normAt, h, h', normSP]
  have hIcc : ∀ j b ξ, v j b ξ ∈ Set.Icc 0 1 := fun j b ξ => by
    rcases hv j b ξ with h | h <;> simp [h]
  rw [outMean]
  simp_rw [hN]
  exact R3_mean side q v hq hIcc

end Star

section Marginal

variable {κ : Type*} [Fintype κ] [DecidableEq κ] {Ω : κ → Type*} [∀ b, Fintype (Ω b)]
  (side : κ → Bool) (q v : ∀ b, Ω b → ℝ)

/-- The 2RSB marginal `E[N^x ρ] / E[N^x]` over the cluster samples of the clauses of `i`.
With `side b = true` on the clauses where `i` is negated, `A = M`, `B = P` and `ρ` is the
`_sT` of `Vertex::compute_s` (`code_p_plus`, `code_site`). -/
noncomputable def marginal (x : ℝ) : ℝ :=
  prodExpect q (fun τ => normSP (sideProd side true (pick v τ)) (sideProd side false (pick v τ))
      ^ x * rho (sideProd side true (pick v τ)) (sideProd side false (pick v τ))) /
    prodExpect q (fun τ =>
      normSP (sideProd side true (pick v τ)) (sideProd side false (pick v τ)) ^ x)

theorem R3_marginal (hq : ∀ b, ∑ ξ, q b ξ = 1) (hv : ∀ b ξ, v b ξ ∈ Set.Icc 0 1) :
    marginal side q v 1 =
      rho (sideProd side true (mean q v)) (sideProd side false (mean q v)) := by
  have hI : ∀ (t : Bool) τ, sideProd side t (pick v τ) ∈ Set.Icc 0 1 := fun t τ =>
    prod_one_sub_mem_Icc _ _ fun b _ => hv b (τ b)
  have h : ∀ τ : (∀ b, Ω b), normSP (sideProd side true (pick v τ))
      (sideProd side false (pick v τ)) ^ (1 : ℝ) *
        rho (sideProd side true (pick v τ)) (sideProd side false (pick v τ)) =
      piU (sideProd side true (pick v τ)) (sideProd side false (pick v τ)) := fun τ => by
    rw [Real.rpow_one]
    exact normSP_mul_rho (hI true τ).1 (hI false τ).1 (hI false τ).2
  simp_rw [marginal, h, Real.rpow_one]
  rw [expect_piU side q v hq, expect_normSP side q v hq]
  rfl

end Marginal

end BspLean
