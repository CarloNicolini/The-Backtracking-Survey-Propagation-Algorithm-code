import BspLean.SP
import Mathlib.Data.Fintype.BigOperators
import Mathlib.Data.Fintype.BigOperators

/-!
# Inner level: SP as a count of sub-states

Fix one cluster. The clause `b` warns the variable `j` in a fraction `η b` of the
sub-states of the cluster, independently for different clauses. A finite weighted
population on each incoming edge models this. The expectation of a product of functions of
independent edges is the product of the expectations (`prodExpect_prod`).

With the 0/1 populations `bern`, expectations are fractions of sub-states. They give
`normSP` (no contradiction at `j`), `piU` (`j` forced against the target clause), and the
SP message as the fraction of the non-contradictory sub-states where every `j` is forced
(`etaOut_counting`).
-/

namespace BspLean

open Finset

section Expect

variable {I : Type*} [Fintype I] [DecidableEq I] {Ω : I → Type*} [∀ i, Fintype (Ω i)]

/-- Expectation of `F` under the product of the weights `p i`. -/
noncomputable def prodExpect (p : ∀ i, Ω i → ℝ) (F : (∀ i, Ω i) → ℝ) : ℝ :=
  ∑ σ, (∏ i, p i (σ i)) * F σ

theorem prodExpect_prod (p f : ∀ i, Ω i → ℝ) :
    prodExpect p (fun σ => ∏ i, f i (σ i)) = ∏ i, ∑ ξ, p i ξ * f i ξ := by
  rw [prodExpect, prod_univ_sum, Fintype.piFinset_univ]
  simp_rw [prod_mul_distrib]

theorem prodExpect_add (p : ∀ i, Ω i → ℝ) (F G : (∀ i, Ω i) → ℝ) :
    prodExpect p (fun σ => F σ + G σ) = prodExpect p F + prodExpect p G := by
  simp only [prodExpect, mul_add, sum_add_distrib]

theorem prodExpect_sub (p : ∀ i, Ω i → ℝ) (F G : (∀ i, Ω i) → ℝ) :
    prodExpect p (fun σ => F σ - G σ) = prodExpect p F - prodExpect p G := by
  simp only [prodExpect, mul_sub, sum_sub_distrib]

end Expect

section Block

variable {κ : Type*} [Fintype κ]

/-- Product of `1 - η b` over the incoming clauses with `side b = t`: `_prod_S` for
`t = true` and `_prod_U` for `t = false`. -/
def sideProd (side : κ → Bool) (t : Bool) (η : κ → ℝ) : ℝ :=
  ∏ b ∈ univ.filter (fun b => side b = t), (1 - η b)

theorem sideProd_eq (side : κ → Bool) (t : Bool) (η : κ → ℝ) :
    sideProd side t η = ∏ b, if side b = t then 1 - η b else 1 := by
  rw [sideProd, prod_filter]

variable {Ω : κ → Type*} [∀ b, Fintype (Ω b)] (side : κ → Bool) (q v : ∀ b, Ω b → ℝ)

/-- Mean of the population of the edge `b`: weights `q b`, values `v b`. -/
noncomputable def mean (b : κ) : ℝ := ∑ ξ, q b ξ * v b ξ

/-- Values of the incoming edges in the joint sample `τ`. -/
def pick (τ : ∀ b, Ω b) : κ → ℝ := fun b => v b (τ b)

variable [DecidableEq κ]

theorem expect_sideProd (hq : ∀ b, ∑ ξ, q b ξ = 1) (t : Bool) :
    prodExpect q (fun τ => sideProd side t (pick v τ)) = sideProd side t (mean q v) := by
  simp_rw [sideProd_eq, pick]
  rw [prodExpect_prod q (fun b ξ => if side b = t then 1 - v b ξ else 1)]
  refine prod_congr rfl fun b _ => ?_
  by_cases h : side b = t <;> simp [h, mean, mul_sub, sum_sub_distrib, hq b]

theorem expect_sideProd_mul (hq : ∀ b, ∑ ξ, q b ξ = 1) :
    prodExpect q (fun τ => sideProd side true (pick v τ) * sideProd side false (pick v τ)) =
      sideProd side true (mean q v) * sideProd side false (mean q v) := by
  simp_rw [sideProd_eq, pick, ← prod_mul_distrib]
  rw [prodExpect_prod q (fun b ξ => (if side b = true then 1 - v b ξ else 1) *
    (if side b = false then 1 - v b ξ else 1))]
  refine prod_congr rfl fun b _ => ?_
  cases h : side b <;> simp [mean, mul_sub, sum_sub_distrib, hq b]

theorem expect_piU (hq : ∀ b, ∑ ξ, q b ξ = 1) :
    prodExpect q (fun τ => piU (sideProd side true (pick v τ)) (sideProd side false (pick v τ)))
      = piU (sideProd side true (mean q v)) (sideProd side false (mean q v)) := by
  have h : ∀ A B : ℝ, piU A B = A - A * B := fun A B => by unfold piU; ring
  simp_rw [h]
  rw [prodExpect_sub, expect_sideProd side q v hq, expect_sideProd_mul side q v hq]

theorem expect_normSP (hq : ∀ b, ∑ ξ, q b ξ = 1) :
    prodExpect q
        (fun τ => normSP (sideProd side true (pick v τ)) (sideProd side false (pick v τ)))
      = normSP (sideProd side true (mean q v)) (sideProd side false (mean q v)) := by
  simp_rw [normSP]
  rw [prodExpect_sub, prodExpect_add, expect_sideProd side q v hq,
    expect_sideProd side q v hq, expect_sideProd_mul side q v hq]

end Block

section Substates

variable {κ : Type*}

/-- Bernoulli weights of the warning of the clause `b`. -/
def bern (η : κ → ℝ) (b : κ) (u : Bool) : ℝ := if u then η b else 1 - η b

/-- The 0/1 value of a warning. -/
def cornerVal (_ : κ) (u : Bool) : ℝ := if u then 1 else 0

theorem sum_bern (η : κ → ℝ) (b : κ) : ∑ u, bern η b u = 1 := by
  simp [bern]

theorem mean_corner (η : κ → ℝ) : mean (bern η) cornerVal = η := by
  funext b; simp [mean, bern, cornerVal]

variable [Fintype κ] (side : κ → Bool)

theorem sideProd_corner (t : Bool) (ω : κ → Bool) :
    sideProd side t (pick cornerVal ω) = if ∃ b, side b = t ∧ ω b = true then 0 else 1 := by
  rw [sideProd_eq]
  split_ifs with h
  · obtain ⟨b, hs, hω⟩ := h
    exact prod_eq_zero (mem_univ b) (by simp [hs, hω, pick, cornerVal])
  · push Not at h
    exact prod_eq_one fun b _ => by by_cases hs : side b = t <;> simp_all [pick, cornerVal]

/-- `1` if the sub-state `ω` of the incoming warnings forces `j` against the target. -/
def forcedIn (ω : κ → Bool) : ℝ :=
  piU (sideProd side true (pick cornerVal ω)) (sideProd side false (pick cornerVal ω))

/-- `1` if the sub-state `ω` of the incoming warnings has no contradiction at `j`. -/
def noContrIn (ω : κ → Bool) : ℝ :=
  normSP (sideProd side true (pick cornerVal ω)) (sideProd side false (pick cornerVal ω))

theorem forced_count (ω : κ → Bool) : forcedIn side ω =
    if (∃ b, side b = false ∧ ω b = true) ∧ ¬∃ b, side b = true ∧ ω b = true then 1 else 0 := by
  rw [forcedIn, sideProd_corner, sideProd_corner]
  split_ifs <;> simp_all [piU]

theorem noContr_count (ω : κ → Bool) : noContrIn side ω =
    if (∃ b, side b = true ∧ ω b = true) ∧ ∃ b, side b = false ∧ ω b = true then 0 else 1 := by
  rw [noContrIn, sideProd_corner, sideProd_corner]
  split_ifs <;> simp_all [normSP]

/-- A forced sub-state has no contradiction, so `forcedIn` counts inside `noContrIn`. -/
theorem forced_mul_noContr (ω : κ → Bool) :
    forcedIn side ω * noContrIn side ω = forcedIn side ω := by
  rw [forced_count, noContr_count]
  split_ifs <;> simp_all

variable [DecidableEq κ]

theorem noContradiction_prob (η : κ → ℝ) :
    prodExpect (bern η) (noContrIn side) =
      normSP (sideProd side true η) (sideProd side false η) := by
  have h := expect_normSP side (bern η) cornerVal (sum_bern η)
  rwa [mean_corner] at h

theorem forced_prob (η : κ → ℝ) :
    prodExpect (bern η) (forcedIn side) = piU (sideProd side true η) (sideProd side false η) := by
  have h := expect_piU side (bern η) cornerVal (sum_bern η)
  rwa [mean_corner] at h

end Substates

section Star

variable {J : Type*} [Fintype J] [DecidableEq J] {κ : J → Type*} [∀ j, Fintype (κ j)]
  [∀ j, DecidableEq (κ j)] (side : ∀ j, κ j → Bool) (η : ∀ j, κ j → ℝ)

/-- Weight of the sub-state `ω` of the incoming warnings of `j`. -/
def subWeight (j : J) (ω : κ j → Bool) : ℝ := ∏ b, bern (η j) b (ω b)

/-- The SP message is the fraction of the non-contradictory sub-states of the cavity
variables where every variable is forced against the target clause. -/
theorem etaOut_counting :
    prodExpect (subWeight η) (fun σ => ∏ j, forcedIn (side j) (σ j)) /
        prodExpect (subWeight η) (fun σ => ∏ j, noContrIn (side j) (σ j)) =
      spMessage (fun j => sideProd (side j) true (η j)) (fun j => sideProd (side j) false (η j))
    := by
  rw [prodExpect_prod, prodExpect_prod, spMessage, ← prod_div_distrib]
  exact prod_congr rfl fun j _ => by
    rw [rho, ← forced_prob (side j) (η j), ← noContradiction_prob (side j) (η j)]; rfl

end Star

end BspLean
