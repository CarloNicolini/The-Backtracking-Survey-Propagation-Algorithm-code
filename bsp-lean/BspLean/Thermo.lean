import BspLean.Update
import Mathlib.Analysis.SpecialFunctions.Exp
import Mathlib.Analysis.SpecialFunctions.Pow.NNReal

/-!
# The inner level at a finite pseudo-temperature: SP-y as a weighted count

Fix one cluster and one cavity variable `j`. A sub-state `ω` of its incoming warnings has
`cntS ω` active warnings from the clauses where `j` has the sign it has in the target clause
(`side b = true`) and `cntU ω` from the others. Its conflict energy is the smaller count, its
weight is `exp (-y · energy)`, and it forces `j` against the target clause if
`cntS ω < cntU ω`. These are the sums `z` and `pi_u` of `thermo_star`
(`include/bsp/thermo_sp.hpp`).

* `thermoPiU_tendsto`, `thermoZ_tendsto`: as `y → ∞` the sums become the SP factors.
* `expect_bern`: the sums are multilinear, so their mean over independent populations is
  their value at the population means.
* `R2y_mean`: on 0/1 populations a cluster has the weight `z(μ₂)^x = exp (-x μ₂ E)`, so the
  outer level is SP-y at `μ₁ = x μ₂`, for every `x`.
* `R3y_mean`, `Zay_mean`: at `x = 1` the means satisfy SP-y at `μ₂`, and so does the clause
  term of `Φ`.
* `R1y_mean`: populations that are single atoms give SP-y at `μ₂`.
-/

namespace BspLean

open Finset Filter Topology

section Star

variable {κ : Type*} [Fintype κ] (side : κ → Bool)

def cntS (ω : κ → Bool) : ℕ := (univ.filter fun b => side b = true ∧ ω b = true).card

def cntU (ω : κ → Bool) : ℕ := (univ.filter fun b => side b = false ∧ ω b = true).card

/-- Conflict energy of the sub-state `ω`. -/
def energy (ω : κ → Bool) : ℕ := min (cntS side ω) (cntU side ω)

/-- `1` if `ω` forces `j` against the target clause. -/
def forcedY (ω : κ → Bool) : ℝ := if cntS side ω < cntU side ω then 1 else 0

noncomputable def boltz (y : ℝ) (ω : κ → Bool) : ℝ := Real.exp (-(y * energy side ω))

variable [DecidableEq κ]

/-- `z` of `thermo_star`. -/
noncomputable def thermoZ (y : ℝ) (η : κ → ℝ) : ℝ := prodExpect (bern η) (boltz side y)

/-- `pi_u` of `thermo_star`. -/
noncomputable def thermoPiU (y : ℝ) (η : κ → ℝ) : ℝ :=
  prodExpect (bern η) (fun ω => boltz side y ω * forcedY side ω)

omit [DecidableEq κ] in
theorem boltz_tendsto (ω : κ → Bool) :
    Tendsto (fun y => boltz side y ω) atTop (𝓝 (if energy side ω = 0 then 1 else 0)) := by
  unfold boltz
  split_ifs with h
  · simp only [h, Nat.cast_zero, mul_zero, neg_zero, Real.exp_zero]
    exact tendsto_const_nhds
  · have hE : (0 : ℝ) < energy side ω := by exact_mod_cast Nat.pos_of_ne_zero h
    exact Real.tendsto_exp_neg_atTop_nhds_zero.comp (tendsto_id.atTop_mul_const hE)

theorem expect_boltz_tendsto (η : κ → ℝ) (F : (κ → Bool) → ℝ) :
    Tendsto (fun y => prodExpect (bern η) (fun ω => boltz side y ω * F ω)) atTop
      (𝓝 (prodExpect (bern η) (fun ω => if energy side ω = 0 then F ω else 0))) := by
  unfold prodExpect
  refine tendsto_finsetSum _ fun ω _ => ?_
  have h := ((boltz_tendsto side ω).mul_const (F ω)).const_mul (∏ b, bern η b (ω b))
  by_cases hE : energy side ω = 0 <;> simpa [hE] using h

omit [DecidableEq κ] in
theorem cntS_eq_zero (ω : κ → Bool) : cntS side ω = 0 ↔ ¬∃ b, side b = true ∧ ω b = true := by
  simp [cntS, filter_eq_empty_iff]

omit [DecidableEq κ] in
theorem cntU_eq_zero (ω : κ → Bool) : cntU side ω = 0 ↔ ¬∃ b, side b = false ∧ ω b = true := by
  simp [cntU, filter_eq_empty_iff]

omit [DecidableEq κ] in
theorem forcedY_ground (ω : κ → Bool) :
    (if energy side ω = 0 then forcedY side ω else 0) = forcedIn side ω := by
  have hS := cntS_eq_zero side ω
  have hU := cntU_eq_zero side ω
  rw [forced_count]
  unfold energy forcedY
  by_cases a : cntS side ω = 0 <;> by_cases b : cntU side ω = 0 <;> simp_all

omit [DecidableEq κ] in
theorem one_ground (ω : κ → Bool) :
    (if energy side ω = 0 then (1 : ℝ) else 0) = noContrIn side ω := by
  have hS := cntS_eq_zero side ω
  have hU := cntU_eq_zero side ω
  rw [noContr_count]
  unfold energy
  by_cases a : cntS side ω = 0 <;> by_cases b : cntU side ω = 0 <;> simp_all

/-- SP-y becomes SP as `y → ∞`: `pi_u → π^U`. -/
theorem thermoPiU_tendsto (η : κ → ℝ) :
    Tendsto (fun y => thermoPiU side y η) atTop
      (𝓝 (piU (sideProd side true η) (sideProd side false η))) := by
  have h := expect_boltz_tendsto side η (forcedY side)
  simp_rw [forcedY_ground] at h
  rwa [forced_prob] at h

/-- SP-y becomes SP as `y → ∞`: `z → N`. -/
theorem thermoZ_tendsto (η : κ → ℝ) :
    Tendsto (fun y => thermoZ side y η) atTop
      (𝓝 (normSP (sideProd side true η) (sideProd side false η))) := by
  have h := expect_boltz_tendsto side η (fun _ => 1)
  simp_rw [mul_one, one_ground] at h
  rwa [noContradiction_prob] at h

theorem thermoZ_pos (y : ℝ) (η : κ → ℝ) (hη : ∀ b, η b ∈ Set.Icc 0 1) :
    0 < thermoZ side y η := by
  have hw : ∀ ω : κ → Bool, 0 ≤ ∏ b, bern η b (ω b) := fun ω =>
    prod_nonneg fun b _ => by cases ω b <;> simp [bern, (hη b).1, (hη b).2]
  have hsum : ∑ ω : κ → Bool, ∏ b, bern η b (ω b) = 1 := by
    have h := prodExpect_prod (bern η) (fun _ _ => (1 : ℝ))
    simp only [prod_const_one, mul_one, sum_bern] at h
    simpa [prodExpect] using h
  refine sum_pos' (fun ω _ => mul_nonneg (hw ω) (Real.exp_pos _).le) ?_
  by_contra h
  push Not at h
  have : ∑ ω : κ → Bool, ∏ b, bern η b (ω b) ≤ 0 :=
    sum_nonpos fun ω _ => nonpos_of_mul_nonpos_left (h ω (mem_univ ω)) (Real.exp_pos _)
  linarith

/-- On a corner `η` the sums are the terms of the single sub-state `ω = (η = 1)`. -/
theorem prodExpect_bern_corner (η : κ → ℝ) (hη : ∀ b, η b = 0 ∨ η b = 1)
    (F : (κ → Bool) → ℝ) : prodExpect (bern η) F = F (fun b => decide (η b = 1)) := by
  have hw : ∀ ω : κ → Bool,
      ∏ b, bern η b (ω b) = if ω = (fun b => decide (η b = 1)) then 1 else 0 := fun ω => by
    split_ifs with h
    · subst h
      exact prod_eq_one fun b _ => by rcases hη b with h | h <;> simp [bern, h]
    · obtain ⟨b, hb⟩ := Function.ne_iff.1 h
      refine prod_eq_zero (mem_univ b) ?_
      rcases hη b with h' | h' <;> cases hω : ω b <;> simp_all [bern]
  simp only [prodExpect, hw, ite_mul, one_mul, zero_mul, sum_ite_eq', mem_univ, ite_true]

variable {Ω : κ → Type*} [∀ b, Fintype (Ω b)] (q v : ∀ b, Ω b → ℝ)

theorem expect_bern_weight (hq : ∀ b, ∑ ξ, q b ξ = 1) (ω : κ → Bool) :
    prodExpect q (fun τ => ∏ b, bern (pick v τ) b (ω b)) = ∏ b, bern (mean q v) b (ω b) := by
  calc prodExpect q (fun τ => ∏ b, bern (pick v τ) b (ω b))
      = ∏ b, ∑ ξ, q b ξ * bern (fun _ => v b ξ) b (ω b) :=
        prodExpect_prod q (fun b ξ => bern (fun _ => v b ξ) b (ω b))
    _ = ∏ b, bern (mean q v) b (ω b) := prod_congr rfl fun b _ => by
        cases ω b <;> simp [bern, mean, mul_sub, sum_sub_distrib, hq b]

/-- The mean over independent populations of a weighted count is its value at the means. -/
theorem expect_bern (hq : ∀ b, ∑ ξ, q b ξ = 1) (F : (κ → Bool) → ℝ) :
    prodExpect q (fun τ => prodExpect (bern (pick v τ)) F) = prodExpect (bern (mean q v)) F := by
  calc prodExpect q (fun τ => prodExpect (bern (pick v τ)) F)
      = ∑ ω, prodExpect q (fun τ => (∏ b, bern (pick v τ) b (ω b)) * F ω) := by
        simp only [prodExpect, mul_sum]
        rw [sum_comm]
    _ = prodExpect (bern (mean q v)) F := by
        simp only [prodExpect_mul_const, expect_bern_weight q v hq]
        rfl

end Star

section Update

variable {J : Type*} [Fintype J] [DecidableEq J] {κ : J → Type*} [∀ j, Fintype (κ j)]
  [∀ j, DecidableEq (κ j)] {Ω : ∀ j, κ j → Type*} [∀ j b, Fintype (Ω j b)]
  (side : ∀ j, κ j → Bool) (q v : ∀ j b, Ω j b → ℝ)

/-- `z` of the cavity variable `j` in the cluster sample `τ`, at the inner parameter `y`. -/
noncomputable def zAt (y : ℝ) (j : J) (τ : ∀ b, Ω j b) : ℝ := thermoZ (side j) y (pick (v j) τ)

noncomputable def piUAtY (y : ℝ) (j : J) (τ : ∀ b, Ω j b) : ℝ :=
  thermoPiU (side j) y (pick (v j) τ)

/-- Mean output of the 2RSB-y update: SP-y at `μ₂` inside each cluster sample, and the
clusters weighted by `(∏_j z_j)^x`. -/
noncomputable def outMeanY (x μ₂ : ℝ) : ℝ :=
  prodExpect (blockW q) (fun σ => (∏ j, zAt side v μ₂ j (σ j)) ^ x *
      ∏ j, piUAtY side v μ₂ j (σ j) / zAt side v μ₂ j (σ j)) /
    prodExpect (blockW q) (fun σ => (∏ j, zAt side v μ₂ j (σ j)) ^ x)

theorem E_prod_thermoZ (hq : ∀ j b, ∑ ξ, q j b ξ = 1) (y : ℝ) :
    prodExpect (blockW q) (fun σ => ∏ j, zAt side v y j (σ j)) =
      ∏ j, thermoZ (side j) y (mean (q j) (v j)) := by
  rw [prodExpect_prod]
  exact prod_congr rfl fun j _ => expect_bern (q j) (v j) (hq j) _

theorem E_prod_thermoPiU (hq : ∀ j b, ∑ ξ, q j b ξ = 1) (y : ℝ) :
    prodExpect (blockW q) (fun σ => ∏ j, piUAtY side v y j (σ j)) =
      ∏ j, thermoPiU (side j) y (mean (q j) (v j)) := by
  rw [prodExpect_prod]
  exact prod_congr rfl fun j _ => expect_bern (q j) (v j) (hq j) _

omit [Fintype J] [DecidableEq J] [∀ j b, Fintype (Ω j b)] in
theorem zAt_pos (hv : ∀ j b ξ, v j b ξ ∈ Set.Icc 0 1) (y : ℝ) (j : J) (τ : ∀ b, Ω j b) :
    0 < zAt side v y j τ :=
  thermoZ_pos (side j) y _ fun b => hv j b (τ b)

/-- R2 at finite `μ`: on 0/1 populations the outer level is SP-y at `μ₁ = x μ₂`. -/
theorem R2y_mean (hq : ∀ j b, ∑ ξ, q j b ξ = 1) (hv : ∀ j b ξ, v j b ξ = 0 ∨ v j b ξ = 1)
    (x μ₂ : ℝ) :
    outMeanY side q v x μ₂ = ∏ j, thermoPiU (side j) (x * μ₂) (mean (q j) (v j)) /
      thermoZ (side j) (x * μ₂) (mean (q j) (v j)) := by
  have hc : ∀ j (τ : ∀ b, Ω j b) (F : (κ j → Bool) → ℝ), prodExpect (bern (pick (v j) τ)) F =
      F (fun b => decide (v j b (τ b) = 1)) := fun j τ F =>
    prodExpect_bern_corner _ (fun b => hv j b (τ b)) F
  have hpow : ∀ j (τ : ∀ b, Ω j b), zAt side v μ₂ j τ ^ x = zAt side v (x * μ₂) j τ :=
    fun j τ => by
      simp only [zAt, thermoZ, hc, boltz, ← Real.exp_mul]
      ring_nf
  have hnn : ∀ j (τ : ∀ b, Ω j b), 0 ≤ zAt side v μ₂ j τ := fun j τ => by
    simp only [zAt, thermoZ, hc, boltz]; exact (Real.exp_pos _).le
  have hden : ∀ σ : (∀ j b, Ω j b),
      (∏ j, zAt side v μ₂ j (σ j)) ^ x = ∏ j, zAt side v (x * μ₂) j (σ j) := fun σ => by
    rw [← Real.finsetProd_rpow _ _ fun j _ => hnn j (σ j)]
    exact prod_congr rfl fun j _ => hpow j (σ j)
  have hnum : ∀ σ : (∀ j b, Ω j b), (∏ j, zAt side v μ₂ j (σ j)) ^ x *
      ∏ j, piUAtY side v μ₂ j (σ j) / zAt side v μ₂ j (σ j) =
        ∏ j, piUAtY side v (x * μ₂) j (σ j) := fun σ => by
    rw [hden, ← prod_mul_distrib]
    refine prod_congr rfl fun j _ => ?_
    rw [← hpow]
    simp only [zAt, piUAtY, thermoZ, thermoPiU, hc, boltz, ← Real.exp_mul]
    field_simp
  simp_rw [outMeanY, hnum, hden]
  rw [E_prod_thermoPiU side q v hq, E_prod_thermoZ side q v hq, ← prod_div_distrib]

/-- R3 at finite `μ`: at `x = 1` the means satisfy SP-y at `μ₂`. -/
theorem R3y_mean (hq : ∀ j b, ∑ ξ, q j b ξ = 1) (hv : ∀ j b ξ, v j b ξ ∈ Set.Icc 0 1)
    (μ₂ : ℝ) :
    outMeanY side q v 1 μ₂ = ∏ j, thermoPiU (side j) μ₂ (mean (q j) (v j)) /
      thermoZ (side j) μ₂ (mean (q j) (v j)) := by
  have hnum : ∀ σ : (∀ j b, Ω j b), (∏ j, zAt side v μ₂ j (σ j)) ^ (1 : ℝ) *
      ∏ j, piUAtY side v μ₂ j (σ j) / zAt side v μ₂ j (σ j) =
        ∏ j, piUAtY side v μ₂ j (σ j) := fun σ => by
    rw [Real.rpow_one, ← prod_mul_distrib]
    exact prod_congr rfl fun j _ => mul_div_cancel₀ _ (zAt_pos side v hv μ₂ j (σ j)).ne'
  simp_rw [outMeanY, hnum, Real.rpow_one]
  rw [E_prod_thermoPiU side q v hq, E_prod_thermoZ side q v hq, ← prod_div_distrib]

/-- The clause term of `Φ` at `x = 1`: `E[∏ z_j - (1 - e^{-μ₂}) ∏ pi_u,j]` is its value at
the means. -/
theorem Zay_mean (hq : ∀ j b, ∑ ξ, q j b ξ = 1) (μ₂ : ℝ) :
    prodExpect (blockW q) (fun σ => ∏ j, zAt side v μ₂ j (σ j) -
        (1 - Real.exp (-μ₂)) * ∏ j, piUAtY side v μ₂ j (σ j)) =
      ∏ j, thermoZ (side j) μ₂ (mean (q j) (v j)) -
        (1 - Real.exp (-μ₂)) * ∏ j, thermoPiU (side j) μ₂ (mean (q j) (v j)) := by
  simp_rw [mul_comm (1 - Real.exp (-μ₂))]
  rw [prodExpect_sub, prodExpect_mul_const, E_prod_thermoZ side q v hq,
    E_prod_thermoPiU side q v hq]

/-- R1 at finite `μ`: single atoms give SP-y at `μ₂`, for every `x`. -/
theorem R1y_mean (η : ∀ j, κ j → ℝ) (hv : ∀ j b ξ, v j b ξ = η j b) (x μ₂ : ℝ)
    (hZ : prodExpect (blockW q) (fun σ => (∏ j, zAt side v μ₂ j (σ j)) ^ x) ≠ 0) :
    outMeanY side q v x μ₂ = ∏ j, thermoPiU (side j) μ₂ (η j) / thermoZ (side j) μ₂ (η j) := by
  have hpick : ∀ j (τ : ∀ b, Ω j b), pick (v j) τ = η j := fun j τ => funext fun b => hv j b (τ b)
  simp only [outMeanY, zAt, piUAtY, hpick] at hZ ⊢
  rw [prodExpect_mul_const, mul_div_cancel_left₀ _ hZ]

end Update

end BspLean
