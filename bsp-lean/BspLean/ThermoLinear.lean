import BspLean.Thermo
import Mathlib.Analysis.Calculus.Deriv.Add
import Mathlib.Analysis.Calculus.Deriv.Inv
import Mathlib.Analysis.Calculus.Deriv.Mul

/-!
# A bug on one edge at a finite pseudo-temperature

Inside a cluster of the corner embedding every incoming warning of `j` is on or off, except
the warning of the bug edge `b`, which is on in a fraction `η b` of the sub-states.

* `prodExpect_bern_update`: the weighted counts are affine in `η b`.
* `nu_bug`: so `ν = pi_u / z` is the two-point ratio `nuTwo` of the sub-states with and
  without the warning of `b`.
* `cnt_flip`: the warning of `b` adds one to the count of its own group.
* `flipU_moves`, `flipS_moves`: the bug moves `forcedY` only in two channels: `b ∈ U` with
  equal counts (channel U) and `b ∈ S` with one more warning in `U` (channel S).
  `nu_const`: elsewhere `ν` does not move.
* `nuU_eq`: in the channel U both sub-states have the same energy and `ν = η b` for every
  `η b`, so the width passes to the output unchanged and on the same corner side.
* `nuS_eq`, `nuS_hasDerivAt_zero`, `nuS_hasDerivAt_one`: in the channel S the energies are
  `k` and `k + 1`. An input near the corner 0 moves the output away from the corner 1 with
  slope `e^{-μ₂}`, and an input near the corner 1 moves it away from the corner 0 with slope
  `e^{μ₂}`.
-/

namespace BspLean

open Finset

section Star

variable {κ : Type*} [Fintype κ] [DecidableEq κ] (side : κ → Bool)

theorem prodExpect_bern_update (η : κ → ℝ) (b : κ) (F : (κ → Bool) → ℝ) :
    prodExpect (bern η) F = η b * prodExpect (bern (Function.update η b 1)) F +
      (1 - η b) * prodExpect (bern (Function.update η b 0)) F := by
  simp only [prodExpect, mul_sum, ← sum_add_distrib]
  refine sum_congr rfl fun ω _ => ?_
  have hsplit : ∀ η' : κ → ℝ, ∏ b', bern η' b' (ω b') =
      bern η' b (ω b) * ∏ b' ∈ univ.erase b, bern η' b' (ω b') := fun η' =>
    (mul_prod_erase univ _ (mem_univ b)).symm
  have hrest : ∀ t : ℝ, ∏ b' ∈ univ.erase b, bern (Function.update η b t) b' (ω b') =
      ∏ b' ∈ univ.erase b, bern η b' (ω b') := fun t => prod_congr rfl fun b' hb' => by
    simp [bern, Function.update_of_ne (ne_of_mem_erase hb')]
  rw [hsplit η, hsplit (Function.update η b 1), hsplit (Function.update η b 0), hrest, hrest]
  cases ω b <;> simp [bern] <;> ring

/-- The sub-state of the corner `update η b t`. -/
noncomputable def cornerOf (η : κ → ℝ) (b : κ) (t : ℝ) : κ → Bool :=
  fun b' => decide (Function.update η b t b' = 1)

/-- `ν` of `j` when the warning of the bug edge is on in a fraction `e` of the sub-states:
the sub-state with the warning has energy `E₁` and `forcedY = f₁`, the other `E₀` and `f₀`. -/
noncomputable def nuTwo (μ₂ E₀ E₁ f₀ f₁ e : ℝ) : ℝ :=
  (e * Real.exp (-(μ₂ * E₁)) * f₁ + (1 - e) * Real.exp (-(μ₂ * E₀)) * f₀) /
    (e * Real.exp (-(μ₂ * E₁)) + (1 - e) * Real.exp (-(μ₂ * E₀)))

theorem nu_bug (η : κ → ℝ) (b : κ) (hη : ∀ b', b' ≠ b → η b' = 0 ∨ η b' = 1) (μ₂ : ℝ) :
    thermoPiU side μ₂ η / thermoZ side μ₂ η =
      nuTwo μ₂ (energy side (cornerOf η b 0)) (energy side (cornerOf η b 1))
        (forcedY side (cornerOf η b 0)) (forcedY side (cornerOf η b 1)) (η b) := by
  have hc : ∀ t : ℝ, (t = 0 ∨ t = 1) → ∀ F : (κ → Bool) → ℝ,
      prodExpect (bern (Function.update η b t)) F = F (cornerOf η b t) := fun t ht F =>
    prodExpect_bern_corner _ (fun b' => by
      by_cases h : b' = b
      · subst h; simpa using ht
      · rw [Function.update_of_ne h]; exact hη b' h) F
  rw [thermoPiU, thermoZ, prodExpect_bern_update _ b,
    prodExpect_bern_update (F := boltz side μ₂) _ b,
    hc 0 (Or.inl rfl), hc 1 (Or.inr rfl), hc 0 (Or.inl rfl), hc 1 (Or.inr rfl), nuTwo]
  simp only [boltz]
  ring_nf

omit [Fintype κ] in
theorem cornerOf_flip (η : κ → ℝ) (b : κ) :
    cornerOf η b 1 = Function.update (cornerOf η b 0) b true ∧ cornerOf η b 0 b = false := by
  refine ⟨funext fun b' => ?_, by simp [cornerOf]⟩
  by_cases h : b' = b
  · subst h; simp [cornerOf]
  · simp [cornerOf, Function.update_of_ne h]

omit [DecidableEq κ] in
theorem cnt_flip [DecidableEq κ] (t : Bool) (ω : κ → Bool) (b : κ) (hb : ω b = false) :
    (univ.filter fun b' => side b' = t ∧ Function.update ω b true b' = true).card =
      (univ.filter fun b' => side b' = t ∧ ω b' = true).card + if side b = t then 1 else 0 := by
  split_ifs with hs
  · rw [← card_insert_of_notMem (s := univ.filter fun b' => side b' = t ∧ ω b' = true)
      (a := b) (by simp [hb])]
    congr 1
    ext b'
    by_cases h : b' = b
    · subst h; simp [hs]
    · simp [h]
  · rw [add_zero]
    congr 1
    ext b'
    by_cases h : b' = b
    · subst h; simp [hs, hb]
    · simp [h]

end Star

/-- A warning of `U` changes `cntS < cntU` only if the counts are equal. -/
theorem flipU_moves (p q : ℕ) : ¬((p < q + 1) ↔ (p < q)) ↔ q = p := by omega

/-- A warning of `S` changes `cntS < cntU` only if `U` has one warning more. -/
theorem flipS_moves (p q : ℕ) : ¬((p + 1 < q) ↔ (p < q)) ↔ q = p + 1 := by omega

section Response

variable (μ₂ : ℝ)

theorem nu_const (E₀ E₁ f e : ℝ)
    (hD : e * Real.exp (-(μ₂ * E₁)) + (1 - e) * Real.exp (-(μ₂ * E₀)) ≠ 0) :
    nuTwo μ₂ E₀ E₁ f f e = f := by
  rw [nuTwo, div_eq_iff hD]; ring

/-- Channel U: `min p (p + 1) = min p p`, so both sub-states weigh the same. -/
theorem nuU_eq (k : ℕ) (e : ℝ) :
    nuTwo μ₂ (min k k : ℕ) (min k (k + 1) : ℕ) 0 1 e = e := by
  have h : ((min k (k + 1) : ℕ) : ℝ) = ((min k k : ℕ) : ℝ) := by simp
  rw [nuTwo, h]
  field_simp
  ring

/-- Channel S: the sub-state without the warning of `b` forces `j` at energy `k`, the one
with it is balanced at energy `k + 1`. -/
theorem nuS_eq (k : ℕ) (e : ℝ) :
    nuTwo μ₂ (min k (k + 1) : ℕ) (min (k + 1) (k + 1) : ℕ) 1 0 e =
      (1 - e) / ((1 - e) + e * Real.exp (-μ₂)) := by
  have h1 : ((min (k + 1) (k + 1) : ℕ) : ℝ) = k + 1 := by simp
  have h0 : ((min k (k + 1) : ℕ) : ℝ) = k := by simp
  have hs : Real.exp (-(μ₂ * (k + 1))) = Real.exp (-(μ₂ * k)) * Real.exp (-μ₂) := by
    rw [← Real.exp_add]; ring_nf
  rw [nuTwo, h1, h0, hs, mul_zero, zero_add, mul_one]
  rw [show e * (Real.exp (-(μ₂ * k)) * Real.exp (-μ₂)) + (1 - e) * Real.exp (-(μ₂ * k)) =
      Real.exp (-(μ₂ * k)) * ((1 - e) + e * Real.exp (-μ₂)) by ring,
    show (1 - e) * Real.exp (-(μ₂ * k)) = Real.exp (-(μ₂ * k)) * (1 - e) by ring,
    mul_div_mul_left _ _ (Real.exp_pos _).ne']

theorem nuS_hasDerivAt (e : ℝ) (hD : (1 - e) + e * Real.exp (-μ₂) ≠ 0) :
    HasDerivAt (fun e => (1 - e) / ((1 - e) + e * Real.exp (-μ₂)))
      ((-1 * ((1 - e) + e * Real.exp (-μ₂)) - (1 - e) * (-1 + Real.exp (-μ₂))) /
        ((1 - e) + e * Real.exp (-μ₂)) ^ 2) e := by
  have hN : HasDerivAt (fun e : ℝ => 1 - e) (-1) e := HasDerivAt.const_sub 1 (hasDerivAt_id' e)
  have hD' := HasDerivAt.add hN (HasDerivAt.mul_const (hasDerivAt_id' e) (Real.exp (-μ₂)))
  rw [one_mul] at hD'
  exact HasDerivAt.div hN hD' hD

/-- Input near the corner 0 (`η b = ε`): the output leaves the corner 1 with slope
`e^{-μ₂}`. -/
theorem nuS_hasDerivAt_zero :
    HasDerivAt (fun e => (1 - e) / ((1 - e) + e * Real.exp (-μ₂))) (-Real.exp (-μ₂)) 0 := by
  have h := nuS_hasDerivAt μ₂ 0 (by simp)
  convert h using 1
  ring

/-- Input near the corner 1 (`η b = 1 - ε`): the output leaves the corner 0 with slope
`e^{μ₂}`. -/
theorem nuS_hasDerivAt_one :
    HasDerivAt (fun e => (1 - e) / ((1 - e) + e * Real.exp (-μ₂))) (-Real.exp μ₂) 1 := by
  have h := nuS_hasDerivAt μ₂ 1 (by simp [(Real.exp_pos _).ne'])
  convert h using 1
  rw [Real.exp_neg]
  field_simp
  ring

end Response

section Channels

variable {κ : Type*} [Fintype κ] [DecidableEq κ] (side : κ → Bool) (η : κ → ℝ) (b : κ)
  (μ₂ : ℝ)

theorem cnt_corner1 :
    cntS side (cornerOf η b 1) = cntS side (cornerOf η b 0) + (if side b = true then 1 else 0) ∧
      cntU side (cornerOf η b 1) =
        cntU side (cornerOf η b 0) + (if side b = false then 1 else 0) := by
  obtain ⟨h1, h0⟩ := cornerOf_flip η b
  rw [h1]
  exact ⟨cnt_flip side true _ b h0, cnt_flip side false _ b h0⟩

/-- Channel U on the star: `b ∈ U` and the other warnings balanced give `ν = η b`. -/
theorem nu_bug_U (hη : ∀ b', b' ≠ b → η b' = 0 ∨ η b' = 1) (hs : side b = false) (k : ℕ)
    (hS : cntS side (cornerOf η b 0) = k)
    (hU : cntU side (cornerOf η b 0) = k) :
    thermoPiU side μ₂ η / thermoZ side μ₂ η = η b := by
  obtain ⟨c1, c2⟩ := cnt_corner1 side η b
  rw [hS, hs] at c1
  rw [hU, hs] at c2
  simp only [Bool.false_eq_true, ↓reduceIte, add_zero] at c1 c2
  rw [nu_bug side η b hη μ₂]
  unfold energy forcedY
  rw [hS, hU, c1, c2]
  simp only [lt_irrefl, Nat.lt_add_one, ↓reduceIte]
  exact nuU_eq μ₂ k (η b)

/-- Channel S on the star: `b ∈ S` and one more warning in `U` give the ratio of `nuS_eq`. -/
theorem nu_bug_S (hη : ∀ b', b' ≠ b → η b' = 0 ∨ η b' = 1) (hs : side b = true) (k : ℕ)
    (hS : cntS side (cornerOf η b 0) = k)
    (hU : cntU side (cornerOf η b 0) = k + 1) :
    thermoPiU side μ₂ η / thermoZ side μ₂ η =
      (1 - η b) / ((1 - η b) + η b * Real.exp (-μ₂)) := by
  obtain ⟨c1, c2⟩ := cnt_corner1 side η b
  rw [hS, hs] at c1
  rw [hU, hs] at c2
  simp only [Bool.true_eq_false, ↓reduceIte, add_zero] at c1 c2
  rw [nu_bug side η b hη μ₂]
  unfold energy forcedY
  rw [hS, hU, c1, c2]
  simp only [lt_irrefl, Nat.lt_add_one, ↓reduceIte]
  exact nuS_eq μ₂ k (η b)

end Channels

end BspLean
