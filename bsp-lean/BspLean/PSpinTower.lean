import Mathlib

/-!+# Finite algebra for the diluted p-spin cavity hierarchy

Companion to `notes/pspin_tower/notes.tex`.
These are identities for finite positive weights. The cavity independence and
pure-state hypotheses, infinite-dimensional measure theory, the thermodynamic
limit, and the full-RSB limit are NOT asserted as Lean theorems here.
-/

namespace PSpinTower

open Finset

noncomputable def spinMessage (x s : ℝ) : ℝ := (1 + s * x) / 2

theorem spinMessage_normalized (x : ℝ) :
    spinMessage x 1 + spinMessage x (-1) = 1 := by
  unfold spinMessage
  ring

theorem spinMessage_nonneg {x s : ℝ} (hx : -1 ≤ x ∧ x ≤ 1)
    (hs : s = 1 ∨ s = -1) : 0 ≤ spinMessage x s := by
  rcases hs with rfl | rfl <;> unfold spinMessage <;> nlinarith [hx.1, hx.2]

/-- `C = cosh (β J)`, `t = tanh (β J)`, `v = ∏ x_j`. -/
theorem factor_normalizer (C t v : ℝ) :
    C * (1 + t * v) + C * (1 - t * v) = 2 * C := by ring

theorem factor_message (C t v s : ℝ) (hC : C ≠ 0) :
    C * (1 + s * t * v) / (2 * C) = spinMessage (t * v) s := by
  unfold spinMessage
  field_simp

theorem variable_message (A B : ℝ) (h : A + B ≠ 0) :
    A / (A + B) = spinMessage ((A - B) / (A + B)) 1 := by
  unfold spinMessage
  field_simp
  ring

section FiniteWeights

variable {ι : Type*} [Fintype ι]

noncomputable def mass (w z : ι → ℝ) (r : ℝ) : ℝ :=
  ∑ i, w i * z i ^ r

noncomputable def tilt (w z : ι → ℝ) (r : ℝ) (i : ι) : ℝ :=
  w i * z i ^ r / mass w z r

theorem mass_pos (w z : ι → ℝ) (r : ℝ)
    (hw : ∀ i, 0 ≤ w i) (ht : ∑ i, w i = 1) (hz : ∀ i, 0 < z i) :
    0 < mass w z r := by
  have hp : 0 < ∑ i, w i := by rw [ht]; norm_num
  obtain ⟨i, hi, hwi⟩ := (Finset.sum_pos_iff_of_nonneg (fun i _ => hw i)).mp hp
  exact Finset.sum_pos' (fun j _ => mul_nonneg (hw j) (Real.rpow_pos_of_pos (hz j) r).le)
    ⟨i, hi, mul_pos hwi (Real.rpow_pos_of_pos (hz i) r)⟩

theorem tilt_normalized (w z : ι → ℝ) (r : ℝ) (h : mass w z r ≠ 0) :
    ∑ i, tilt w z r i = 1 := by
  simp only [tilt, ← sum_div]
  exact div_self h

theorem tilt_nonneg (w z : ι → ℝ) (r : ℝ)
    (hw : ∀ i, 0 ≤ w i) (hz : ∀ i, 0 ≤ z i) (i : ι) :
    0 ≤ tilt w z r i := by
  exact div_nonneg (mul_nonneg (hw i) (Real.rpow_nonneg (hz i) r))
    (sum_nonneg fun j _ => mul_nonneg (hw j) (Real.rpow_nonneg (hz j) r))

theorem mass_constant (w : ι → ℝ) (ht : ∑ i, w i = 1) (c r : ℝ) :
    mass w (fun _ => c) r = c ^ r := by
  simp [mass, ← sum_mul, ht]

/-- A constant positive reweighting cancels, as at a p-spin factor node. -/
theorem tilt_constant (w : ι → ℝ) (ht : ∑ i, w i = 1) (c r : ℝ)
    (hc : 0 < c) (i : ι) : tilt w (fun _ => c) r i = w i := by
  rw [tilt, mass_constant w ht]
  exact mul_div_cancel_right₀ _ (Real.rpow_pos_of_pos hc r).ne'

theorem mass_scale (w z : ι → ℝ) (r c : ℝ) (hc : 0 ≤ c) (hz : ∀ i, 0 ≤ z i) :
    mass w (fun i => c * z i) r = c ^ r * mass w z r := by
  simp only [mass, Real.mul_rpow hc (hz _), mul_sum]
  apply sum_congr rfl
  intro i _
  ring

end FiniteWeights

/-- Independent incoming probability weights have total mass one. -/
theorem product_weights_normalized {κ ι : Type*} [Fintype κ] [DecidableEq κ] [Fintype ι]
    (w : κ → ι → ℝ) (ht : ∀ j, ∑ i, w j i = 1) :
    ∑ choices : κ → ι, ∏ j, w j (choices j) = 1 := by
  rw [← Fintype.prod_sum]
  simp [ht]

def spin (s : Bool) : ℝ := if s then 1 else -1

theorem spin_sum (x : ℝ) : ∑ s : Bool, spinMessage x (spin s) = 1 := by
  simp [spin, spinMessage]
  ring

theorem spin_first_moment (x : ℝ) :
    ∑ s : Bool, spinMessage x (spin s) * spin s = x := by
  simp [spin, spinMessage]
  ring

/-- The spin sum used to derive the p-spin factor map, for arbitrary finite arity. -/
theorem product_spin_moment {κ : Type*} [Fintype κ] [DecidableEq κ] (x : κ → ℝ) :
    ∑ conf : κ → Bool, (∏ j, spinMessage (x j) (spin (conf j))) *
        (∏ j, spin (conf j)) = ∏ j, x j := by
  simp_rw [← Finset.prod_mul_distrib]
  rw [← Fintype.prod_sum (fun j (s : Bool) => spinMessage (x j) (spin s) * spin s)]
  simp_rw [spin_first_moment]

/-- The exponent forced by a singleton inner level. -/
theorem power_ratio (z a b : ℝ) (hz : 0 ≤ z) (hb : b ≠ 0) :
    (z ^ b) ^ (a / b) = z ^ a := by
  rw [← Real.rpow_mul hz]
  congr 1
  field_simp

section NestedStates

variable {A Γ : Type*} [Fintype A] [Fintype Γ]

noncomputable def inner (Z : A → Γ → ℝ) (m₂ : ℝ) (a : A) : ℝ :=
  ∑ g, Z a g ^ m₂

noncomputable def stateSum2 (Z : A → Γ → ℝ) (m₁ m₂ : ℝ) : ℝ :=
  ∑ a, inner Z m₂ a ^ (m₁ / m₂)

omit [Fintype A] in
/-- The finite state-sum identity that produces the inner normalizer. -/
theorem inner_reweight (Z z : A → Γ → ℝ) (m₂ : ℝ) (a : A)
    (hZ : ∀ g, 0 ≤ Z a g) (hz : ∀ g, 0 ≤ z a g)
    (hW : inner Z m₂ a ≠ 0) :
    inner (fun a g => Z a g * z a g) m₂ a =
      inner Z m₂ a * mass (fun g => Z a g ^ m₂ / inner Z m₂ a) (z a) m₂ := by
  simp only [inner, mass, Real.mul_rpow (hZ _) (hz _), mul_sum]
  apply sum_congr rfl
  intro g _
  have hW' : ∑ g, Z a g ^ m₂ ≠ 0 := hW
  field_simp

/-- Multiplication of a cluster's inner sum by C tilts its outer weight by C^r. -/
theorem outer_reweight {ι : Type*} [Fintype ι] (W C : ι → ℝ) (r : ℝ)
    (hW : ∀ i, 0 ≤ W i) (hC : ∀ i, 0 ≤ C i)
    (hY : ∑ i, W i ^ r ≠ 0) (i : ι) :
    (W i * C i) ^ r / ∑ j, (W j * C j) ^ r =
      tilt (fun j => W j ^ r / ∑ k, W k ^ r) C r i := by
  simp only [tilt, mass, Real.mul_rpow (hW _) (hC _)]
  simp_rw [div_mul_eq_mul_div, ← sum_div]
  rw [div_div_div_cancel_right₀ hY]

theorem equal_parameters (Z : A → Γ → ℝ) (m : ℝ) (hm : m ≠ 0) :
    stateSum2 Z m m = ∑ a, ∑ g, Z a g ^ m := by
  simp [stateSum2, inner, div_self hm]

theorem one_state_per_family (Z : A → Unit → ℝ) (m₁ m₂ : ℝ)
    (hZ : ∀ a, 0 ≤ Z a ()) (hm₂ : m₂ ≠ 0) :
    stateSum2 Z m₁ m₂ = ∑ a, Z a () ^ m₁ := by
  simp only [stateSum2, inner, Fintype.sum_unique]
  apply sum_congr rfl
  intro a _
  exact power_ratio _ _ _ (hZ a) hm₂

end NestedStates

/-- With equal adjacent parameters, the inner normalizer cancels in the joint weight.
Summing this identity over atoms with the same outgoing message gives the barycenter
collapse of 2RSB to 1RSB, not an equality of the unflattened message types. -/
theorem flatten_weight (v p z C Y : ℝ) (hC : C ≠ 0) :
    (v * C / Y) * (p * z / C) = v * p * z / Y := by
  field_simp

/-- Finite two-level entropy decomposition, using L=E log W and f=E log Z. -/
theorem entropy_decomposition (phi L f m₁ m₂ : ℝ) (hm₂ : m₂ ≠ 0) :
    phi = (phi - m₁ / m₂ * L) + m₁ / m₂ * (L - m₂ * f) + m₁ * f := by
  field_simp
  ring

/-- An atomic probability law with explicit nonnegative, normalized weights. -/
structure FiniteLaw (X : Type) where
  size : ℕ
  value : Fin size → X
  weight : Fin size → ℝ
  nonneg : ∀ i, 0 ≤ weight i
  total : ∑ i, weight i = 1

/-- Level zero can be a magnetization; every successor adds one state-law level. -/
def ProbabilityTower (X : Type) : ℕ → Type
  | 0 => X
  | k + 1 => FiniteLaw (ProbabilityTower X k)

/-- One normalized, positive weighted pushforward: the building block at every level. -/
noncomputable def liftLaw {X Y : Type} (q : FiniteLaw X) (f : X → Y)
    (z : X → ℝ) (r : ℝ) (hz : ∀ x, 0 < z x) : FiniteLaw Y where
  size := q.size
  value i := f (q.value i)
  weight := tilt q.weight (fun i => z (q.value i)) r
  nonneg := tilt_nonneg _ _ _ q.nonneg (fun i => (hz (q.value i)).le)
  total := tilt_normalized _ _ _
    (mass_pos _ _ _ q.nonneg q.total (fun i => hz (q.value i))).ne'

/-- A finite tree of positive local factors and normalized probability averages.
At successive depths its exponents are m_K, m_(K-1)/m_K, ..., m_1/m_2.
Allowing arbitrary real exponents makes the positivity theorem stronger. -/
inductive NormalizerTree where
  | leaf (z : ℝ) (positive : 0 < z)
  | node (n : ℕ) (w : Fin n → ℝ) (nonneg : ∀ i, 0 ≤ w i)
      (total : ∑ i, w i = 1) (children : Fin n → NormalizerTree) (r : ℝ)

noncomputable def normalizer : NormalizerTree → ℝ
  | .leaf z _ => z
  | .node _ w _ _ children r => mass w (fun i => normalizer (children i)) r

theorem normalizer_pos (t : NormalizerTree) : 0 < normalizer t := by
  induction t with
  | leaf z hz => exact hz
  | node n w hw ht children r ih => exact mass_pos _ _ _ hw ht ih

#print axioms factor_message
#print axioms inner_reweight
#print axioms outer_reweight
#print axioms equal_parameters
#print axioms one_state_per_family
#print axioms flatten_weight
#print axioms liftLaw
#print axioms normalizer_pos

end PSpinTower
