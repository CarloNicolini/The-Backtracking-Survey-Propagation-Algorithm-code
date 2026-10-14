import BspLean.Update
import Mathlib.Analysis.Calculus.Deriv.Add
import Mathlib.Analysis.Calculus.Deriv.Inv
import Mathlib.Analysis.Calculus.Deriv.Mul

/-!
# Linearization of the 2RSB update at the corner embedding

Type II perturbation: one incoming edge `b → j` of the target clause gets the value `1 - ε`
instead of `1` and `ε` instead of `0` (`bugVal`). This is a "bug": in a fraction `ε` of the
sub-states of a cluster, the warning of `b` flips.

* `rho_hasDerivAt`, `T_coefficient`: `∂ρ_j/∂η_b = A_j B'_j / N_j²` for `b` in the opposite
  sign group `U`, where `B_j = (1 - η_b) B'_j`. The stability coefficient is `N_j ∂ρ_j/∂η_b`.
* `bug_block`: for `b ∈ U`, the output of `j` moves by exactly `ε A_j B'_j` from its corner,
  with weight `N_j^x = 1`. The non-analytic weights `N_j^x = ε^x` only multiply outputs
  that stay exactly on a corner.
* `bug_block_S`: for `b ∈ S` the output of `j` stays on a corner, so no width propagates.
-/

namespace BspLean

open Finset

/-- Value of the bug edge: `1 - ε` near the corner `1` and `ε` near the corner `0`. -/
def bugVal (ε : ℝ) (u : Bool) : ℝ := if u then 1 - ε else ε

/-- Distance of `y` from the nearest corner. -/
noncomputable def dev (y : ℝ) : ℝ := min y (1 - y)

section Analytic

variable (A B' : ℝ)

theorem rho_hasDerivAt (e : ℝ) (hN : normSP A ((1 - e) * B') ≠ 0) :
    HasDerivAt (fun e => rho A ((1 - e) * B')) (A * B' / normSP A ((1 - e) * B') ^ 2) e := by
  have hf : HasDerivAt (fun e => piU A ((1 - e) * B')) (A * B') e := by
    rw [show (fun e => piU A ((1 - e) * B')) = fun e => (A - A * B') + A * B' * e by
      funext e; unfold piU; ring]
    have h := HasDerivAt.const_add (A - A * B') (HasDerivAt.const_mul (A * B') (hasDerivAt_id' e))
    rwa [mul_one] at h
  have hg : HasDerivAt (fun e => normSP A ((1 - e) * B')) (-(B' * (1 - A))) e := by
    rw [show (fun e => normSP A ((1 - e) * B')) = fun e => (A + B' - A * B') + -(B' * (1 - A)) * e
      by funext e; unfold normSP; ring]
    have h := HasDerivAt.const_add (A + B' - A * B')
      (HasDerivAt.const_mul (-(B' * (1 - A))) (hasDerivAt_id' e))
    rwa [mul_one] at h
  refine HasDerivAt.congr_deriv (HasDerivAt.div hf hg hN) ?_
  congr 1
  unfold piU normSP; ring

/-- The stability coefficient is `N_j` times the Jacobian of SP. -/
theorem T_coefficient (e : ℝ) :
    normSP A ((1 - e) * B') * (A * B' / normSP A ((1 - e) * B') ^ 2) =
      A * B' / normSP A ((1 - e) * B') := by
  rcases eq_or_ne (normSP A ((1 - e) * B')) 0 with h | h
  · simp [h]
  · field_simp

end Analytic

section Bug

variable {A B' : ℝ} (ε x : ℝ)

theorem bug_block (hA : A = 0 ∨ A = 1) (hB : B' = 0 ∨ B' = 1) (u : Bool) (hε : ε ≤ 1 / 2) :
    normSP A ((1 - bugVal ε u) * B') ^ x * dev (rho A ((1 - bugVal ε u) * B')) =
      ε * (A * B') := by
  rcases hA with rfl | rfl
  · simp [rho, piU, dev]
  rcases hB with rfl | rfl
  · simp [rho, piU, normSP, dev]
  have hN : normSP 1 ((1 - bugVal ε u) * 1) = 1 := by unfold normSP; ring
  rw [rho, hN, div_one, Real.one_rpow, one_mul, mul_one, mul_one]
  cases u <;> simp only [piU, bugVal, dev, Bool.false_eq_true, ite_true, ite_false] <;>
    [rw [min_eq_left (by linarith)]; rw [min_eq_right (by linarith)]] <;> ring

/-- A bug on an edge of the same sign group `S` scales `A` by `c = 1 - bugVal ε u`. -/
theorem bug_block_S (c : ℝ) (hA : A = 0 ∨ A = 1) {B : ℝ} (hB : B = 0 ∨ B = 1) :
    dev (rho (c * A) B) = 0 := by
  rcases hA with rfl | rfl <;> rcases hB with rfl | rfl <;>
    rcases eq_or_ne c 0 with hc | hc <;> simp [rho, piU, normSP, dev, hc]

end Bug

section Corner

variable {κ : Type*} [Fintype κ] (side : κ → Bool)

/-- The SP factor of `j` in a sub-state `ω` of its incoming warnings. -/
noncomputable def rhoIn (ω : κ → Bool) : ℝ :=
  rho (sideProd side true (pick cornerVal ω)) (sideProd side false (pick cornerVal ω))

theorem rhoIn_eq_forcedIn (ω : κ → Bool) : rhoIn side ω = forcedIn side ω := by
  rw [rhoIn, forcedIn, sideProd_corner, sideProd_corner]
  split_ifs <;> simp [rho, piU, normSP]

theorem forcedIn_bit (ω : κ → Bool) : forcedIn side ω = 0 ∨ forcedIn side ω = 1 := by
  rw [forced_count]; split_ifs <;> simp

theorem noContrIn_nonneg (ω : κ → Bool) : 0 ≤ noContrIn side ω := by
  rw [noContr_count]; split_ifs <;> simp

end Corner

section Star

variable {κ0 : Type*} [Fintype κ0] (side0 : κ0 → Bool)
  {J : Type*} [Fintype J] {κ : J → Type*} [∀ j, Fintype (κ j)] (side : ∀ j, κ j → Bool)
  (ε x : ℝ)

/-- Weighted distance from the corner of the output of one cluster sample: `u` is the bug
edge, `ω0` the other incoming warnings of its variable, `σ` those of the other variables. -/
noncomputable def bugTerm (u : Bool) (ω0 : κ0 → Bool) (σ : ∀ j, κ j → Bool) : ℝ :=
  (normSP (sideProd side0 true (pick cornerVal ω0))
      ((1 - bugVal ε u) * sideProd side0 false (pick cornerVal ω0)) *
    ∏ j, noContrIn (side j) (σ j)) ^ x *
  dev (rho (sideProd side0 true (pick cornerVal ω0))
      ((1 - bugVal ε u) * sideProd side0 false (pick cornerVal ω0)) *
    ∏ j, rhoIn (side j) (σ j))

theorem bugTerm_eq (hε : ε ≤ 1 / 2) (u : Bool) (ω0 : κ0 → Bool) (σ : ∀ j, κ j → Bool) :
    bugTerm side0 side ε x u ω0 σ = ε * (sideProd side0 true (pick cornerVal ω0) *
      sideProd side0 false (pick cornerVal ω0)) * ∏ j, forcedIn (side j) (σ j) := by
  have hbit : ∀ t, sideProd side0 t (pick cornerVal ω0) = 0 ∨
      sideProd side0 t (pick cornerVal ω0) = 1 := fun t => by
    rw [sideProd_corner]; split_ifs <;> simp
  have hMR : (∏ j, noContrIn (side j) (σ j)) * ∏ j, forcedIn (side j) (σ j) =
      ∏ j, forcedIn (side j) (σ j) := by
    rw [← prod_mul_distrib]
    exact prod_congr rfl fun j _ => by rw [mul_comm, forced_mul_noContr]
  simp_rw [bugTerm, rhoIn_eq_forcedIn]
  rcases prod_bit univ _ (fun j _ => forcedIn_bit (side j) (σ j)) with h | h
  · simp [h, dev]
  · rw [h, mul_one] at hMR
    rw [h, hMR, mul_one, mul_one, mul_one]
    exact bug_block ε x (hbit true) (hbit false) u hε

end Star

end BspLean
