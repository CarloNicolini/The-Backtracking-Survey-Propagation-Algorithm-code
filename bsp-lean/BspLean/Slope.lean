import BspLean.Linear
import Mathlib.Analysis.SpecialFunctions.Pow.Continuity
import Mathlib.Analysis.Calculus.Deriv.Slope

/-!
# Slope of the output width at the corner embedding

All incoming edges carry 0/1 populations (the corner embedding, R2), except the bug edge
`b → j`, whose members sit at distance `ε` from their corners (input width `ε`). The output
width is `widthNum ε / widthDen ε`: the expected distance of the output from its corner
under the 2RSB weights `(∏ N)^x`.

* `widthNum_eq`: for `ε ≤ 1/2` the numerator is exactly `ε · C`, with no `ε^x` term.
* `slope_hasDerivAt`: the output width has derivative `C / widthDen 0` at `ε = 0`.
* `coef_code`: this derivative is the coefficient of the C++ stability test,
  `∏_{c ∈ ∂j∖{a,b}} (1 - η_c) / N_j · ∏_{j' ≠ j} ρ_{j'}`.
-/

namespace BspLean

open Finset Filter Topology

theorem hasDerivAt_mul_of_continuousAt (g : ℝ → ℝ) (hg : ContinuousAt g 0) :
    HasDerivAt (fun ε => ε * g ε) (g 0) 0 := by
  rw [hasDerivAt_iff_tendsto_slope]
  have h : slope (fun ε => ε * g ε) 0 =ᶠ[𝓝[≠] 0] g := by
    filter_upwards [self_mem_nhdsWithin] with ε (hε : ε ≠ 0)
    rw [slope_def_field]; field_simp; ring
  exact (hg.tendsto.mono_left nhdsWithin_le_nhds).congr' h.symm

theorem prodExpect_const_mul {I : Type*} [Fintype I] [DecidableEq I] {Ξ : I → Type*}
    [∀ i, Fintype (Ξ i)] (p : ∀ i, Ξ i → ℝ) (F : (∀ i, Ξ i) → ℝ) (c : ℝ) :
    prodExpect p (fun σ => c * F σ) = c * prodExpect p F := by
  simp only [prodExpect, mul_sum, mul_left_comm]

section Star

variable {κ0 : Type*} [Fintype κ0] [DecidableEq κ0] (side0 : κ0 → Bool) (η0 : κ0 → ℝ) (ηb : ℝ)
  {J : Type*} [Fintype J] [DecidableEq J] {κ : J → Type*} [∀ j, Fintype (κ j)]
  [∀ j, DecidableEq (κ j)] (side : ∀ j, κ j → Bool) (η : ∀ j, κ j → ℝ) (x : ℝ)

/-- Expectation over the corner populations: the bug edge (mass `ηb` at `1`), the other
incoming edges of its variable (`η0`) and those of the other variables (`η`). -/
noncomputable def starExpect (F : Bool → (κ0 → Bool) → (∀ j, κ j → Bool) → ℝ) : ℝ :=
  ∑ u, (if u then ηb else 1 - ηb) *
    prodExpect (bern η0) (fun ω0 => prodExpect (subWeight η) (fun σ => F u ω0 σ))

noncomputable def widthNum (ε : ℝ) : ℝ := starExpect η0 ηb η (bugTerm side0 side ε x)

noncomputable def widthDen (ε : ℝ) : ℝ :=
  starExpect η0 ηb η fun u ω0 σ => (normSP (sideProd side0 true (pick cornerVal ω0))
    ((1 - bugVal ε u) * sideProd side0 false (pick cornerVal ω0)) *
      ∏ j, noContrIn (side j) (σ j)) ^ x

/-- Numerator of the slope: the probability that the bug edge alone forces `j` while every
other variable of the clause is forced. -/
noncomputable def channel : ℝ :=
  sideProd side0 true η0 * sideProd side0 false η0 *
    ∏ j, piU (sideProd (side j) true (η j)) (sideProd (side j) false (η j))

theorem E_prod_forced : prodExpect (subWeight η) (fun σ => ∏ j, forcedIn (side j) (σ j)) =
    ∏ j, piU (sideProd (side j) true (η j)) (sideProd (side j) false (η j)) := by
  rw [prodExpect_prod]
  exact prod_congr rfl fun j _ => forced_prob (side j) (η j)

theorem E_prod_noContr : prodExpect (subWeight η) (fun σ => ∏ j, noContrIn (side j) (σ j)) =
    ∏ j, normSP (sideProd (side j) true (η j)) (sideProd (side j) false (η j)) := by
  rw [prodExpect_prod]
  exact prod_congr rfl fun j _ => noContradiction_prob (side j) (η j)

theorem E_side0 (t : Bool) :
    prodExpect (bern η0) (fun ω0 => sideProd side0 t (pick cornerVal ω0)) =
      sideProd side0 t η0 := by
  rw [expect_sideProd side0 (bern η0) cornerVal (sum_bern η0), mean_corner]

theorem E_side0_mul : prodExpect (bern η0) (fun ω0 => sideProd side0 true (pick cornerVal ω0) *
    sideProd side0 false (pick cornerVal ω0)) =
      sideProd side0 true η0 * sideProd side0 false η0 := by
  rw [expect_sideProd_mul side0 (bern η0) cornerVal (sum_bern η0), mean_corner]

theorem widthNum_eq (ε : ℝ) (hε : ε ≤ 1 / 2) :
    widthNum side0 η0 ηb side η x ε = ε * channel side0 η0 side η := by
  simp_rw [widthNum, starExpect, bugTerm_eq side0 side ε x hε, prodExpect_const_mul,
    E_prod_forced, prodExpect_mul_const, prodExpect_const_mul, E_side0_mul, channel]
  simp; ring

theorem E_normSP_scaled (c : ℝ) :
    prodExpect (bern η0) (fun ω0 => normSP (sideProd side0 true (pick cornerVal ω0))
      (c * sideProd side0 false (pick cornerVal ω0))) =
      normSP (sideProd side0 true η0) (c * sideProd side0 false η0) := by
  have h : ∀ A B : ℝ, normSP A (c * B) = A + c * B - c * (A * B) := fun A B => by
    unfold normSP; ring
  simp_rw [h]
  rw [prodExpect_sub, prodExpect_add, prodExpect_const_mul, prodExpect_const_mul, E_side0,
    E_side0, E_side0_mul]

theorem widthDen_zero (hx : x ≠ 0) : widthDen side0 η0 ηb side η x 0 =
    normSP (sideProd side0 true η0) ((1 - ηb) * sideProd side0 false η0) *
      ∏ j, normSP (sideProd (side j) true (η j)) (sideProd (side j) false (η j)) := by
  have hbit : ∀ c : ℝ, (c = 0 ∨ c = 1) → ∀ (ω0 : κ0 → Bool) (σ : ∀ j, κ j → Bool),
      (normSP (sideProd side0 true (pick cornerVal ω0))
        (c * sideProd side0 false (pick cornerVal ω0)) * ∏ j, noContrIn (side j) (σ j)) ^ x =
      normSP (sideProd side0 true (pick cornerVal ω0))
        (c * sideProd side0 false (pick cornerVal ω0)) * ∏ j, noContrIn (side j) (σ j) :=
    fun c hc ω0 σ => by
      have hN : normSP (sideProd side0 true (pick cornerVal ω0))
          (c * sideProd side0 false (pick cornerVal ω0)) = 0 ∨
          normSP (sideProd side0 true (pick cornerVal ω0))
            (c * sideProd side0 false (pick cornerVal ω0)) = 1 := by
        rw [sideProd_corner, sideProd_corner]
        rcases hc with rfl | rfl <;> split_ifs <;> simp [normSP]
      have hM := prod_bit univ _ fun j _ => (show noContrIn (side j) (σ j) = 0 ∨
        noContrIn (side j) (σ j) = 1 by rw [noContr_count]; split_ifs <;> simp)
      refine bit_rpow x hx ?_
      rcases hN with h | h <;> rcases hM with h' | h' <;> simp [h, h']
  have key : ∀ u, prodExpect (bern η0) (fun ω0 => prodExpect (subWeight η) (fun σ =>
      (normSP (sideProd side0 true (pick cornerVal ω0))
        ((1 - bugVal 0 u) * sideProd side0 false (pick cornerVal ω0)) *
          ∏ j, noContrIn (side j) (σ j)) ^ x)) =
      normSP (sideProd side0 true η0) ((1 - bugVal 0 u) * sideProd side0 false η0) *
        ∏ j, normSP (sideProd (side j) true (η j)) (sideProd (side j) false (η j)) := fun u => by
    have hc : 1 - bugVal 0 u = 0 ∨ 1 - bugVal 0 u = 1 := by cases u <;> simp [bugVal]
    simp_rw [hbit _ hc, prodExpect_const_mul, E_prod_noContr, prodExpect_mul_const,
      E_normSP_scaled]
  simp_rw [widthDen, starExpect, key]
  rw [Fintype.sum_bool]
  simp only [bugVal, ↓reduceIte, Bool.false_eq_true]
  unfold normSP; ring

theorem widthDen_continuous (hx : 0 < x) : Continuous (widthDen side0 η0 ηb side η x) := by
  have hb : ∀ u, Continuous fun ε => bugVal ε u := fun u => by
    cases u <;> simp only [bugVal, ↓reduceIte, Bool.false_eq_true] <;> fun_prop
  unfold widthDen starExpect prodExpect
  refine continuous_finsetSum _ fun u _ => continuous_const.mul <| continuous_finsetSum _
    fun ω0 _ => continuous_const.mul <| continuous_finsetSum _ fun σ _ =>
      continuous_const.mul <| (Real.continuous_rpow_const hx.le).comp ?_
  have := hb u
  unfold normSP
  fun_prop

/-- The output width has slope `channel / widthDen 0` in the input width `ε`. -/
theorem slope_hasDerivAt (hx : 0 < x) (hD : widthDen side0 η0 ηb side η x 0 ≠ 0) :
    HasDerivAt (fun ε => widthNum side0 η0 ηb side η x ε / widthDen side0 η0 ηb side η x ε)
      (channel side0 η0 side η / widthDen side0 η0 ηb side η x 0) 0 := by
  have hg : ContinuousAt (fun ε => channel side0 η0 side η / widthDen side0 η0 ηb side η x ε) 0 :=
    continuousAt_const.div (widthDen_continuous side0 η0 ηb side η x hx).continuousAt hD
  refine HasDerivAt.congr_of_eventuallyEq (hasDerivAt_mul_of_continuousAt _ hg) ?_
  filter_upwards [Iio_mem_nhds (show (0 : ℝ) < 1 / 2 by norm_num)] with ε hε
  rw [widthNum_eq side0 η0 ηb side η x ε (le_of_lt hε), mul_div_assoc]

/-- The slope is the coefficient `T` of `src/rsb2_graph.cpp`. -/
theorem coef_code (hx : x ≠ 0) :
    channel side0 η0 side η / widthDen side0 η0 ηb side η x 0 =
      sideProd side0 true η0 * sideProd side0 false η0 /
          normSP (sideProd side0 true η0) ((1 - ηb) * sideProd side0 false η0) *
        spMessage (fun j => sideProd (side j) true (η j))
          (fun j => sideProd (side j) false (η j)) := by
  rw [channel, widthDen_zero side0 η0 ηb side η x hx, mul_div_mul_comm, spMessage,
    ← prod_div_distrib]
  rfl

end Star

end BspLean
