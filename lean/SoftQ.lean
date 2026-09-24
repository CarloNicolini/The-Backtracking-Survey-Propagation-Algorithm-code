/-
Soft two-step look-ahead for BSP: the algebraic facts used in
paper/sections/softq.tex. Every statement is proved without `sorry`.

Notation of the paper:
* `lnq q x`            : Tsallis q-logarithm, the per-move reward is `lnq q b`.
* `soft2 α a b`        : `α log (e^{a/α} + e^{b/α})`, the soft value `v_α` of one site.
* `soft α Q`           : `α log Σ_a e^{Q_a/α}`, the soft Bellman value over actions.
-/
import Mathlib

open Real Finset Filter Topology

namespace SoftQ

/-! ## 1. Telescoping of the complexity reward -/

/-- The return of the reward `ΔΣ` along a path depends only on its endpoints. -/
theorem telescoping (S : ℕ → ℝ) (n : ℕ) :
    ∑ t ∈ range n, (S (t + 1) - S t) = S n - S 0 :=
  Finset.sum_range_sub S n

/-- Two paths with the same endpoints have the same `ΔΣ` return. -/
theorem same_return (S S' : ℕ → ℝ) (n : ℕ) (h0 : S 0 = S' 0) (hn : S n = S' n) :
    ∑ t ∈ range n, (S (t + 1) - S t) = ∑ t ∈ range n, (S' (t + 1) - S' t) := by
  rw [telescoping, telescoping, h0, hn]

/-! ## 2. The Tsallis reward -/

/-- Tsallis q-logarithm `ln_q x = (x^{1-q} - 1)/(1 - q)`. -/
noncomputable def lnq (q x : ℝ) : ℝ := (x ^ (1 - q) - 1) / (1 - q)

theorem lnq_one (q : ℝ) : lnq q 1 = 0 := by simp [lnq]

/-- Pseudo-additivity of `ln_q`. -/
theorem lnq_mul (q x y : ℝ) (hq : q ≠ 1) (hx : 0 < x) (hy : 0 < y) :
    lnq q (x * y) = lnq q x + lnq q y + (1 - q) * lnq q x * lnq q y := by
  have hc : (1 - q) ≠ 0 := sub_ne_zero.mpr (Ne.symm hq)
  unfold lnq
  rw [Real.mul_rpow hx.le hy.le]
  field_simp
  ring

/-- For `q > 1` and `0 < x < 1` the reward is negative. -/
theorem lnq_neg (q x : ℝ) (hq : 1 < q) (hx : 0 < x) (hx1 : x < 1) : lnq q x < 0 := by
  unfold lnq
  have h1 : 1 < x ^ (1 - q) := Real.one_lt_rpow_of_pos_of_lt_one_of_neg hx hx1 (by linarith)
  apply div_neg_of_pos_of_neg <;> linarith

/-- For `q > 1` two small drops are worth more than one drop with the same product. -/
theorem lnq_split (q x y : ℝ) (hq : 1 < q) (hx : 0 < x) (hx1 : x < 1)
    (hy : 0 < y) (hy1 : y < 1) :
    lnq q (x * y) < lnq q x + lnq q y := by
  rw [lnq_mul q x y (ne_of_gt hq) hx hy]
  have ha := lnq_neg q x hq hx hx1
  have hb := lnq_neg q y hq hy hy1
  have : (1 - q) * lnq q x * lnq q y < 0 := by
    have hp : 0 < lnq q x * lnq q y := mul_pos_of_neg_of_neg ha hb
    have : (1 - q) * (lnq q x * lnq q y) < 0 := mul_neg_of_neg_of_pos (by linarith) hp
    linarith [mul_assoc (1 - q) (lnq q x) (lnq q y)]
  linarith

/-- For `q > 1` the reward is strictly increasing: at one step it ranks like the bias. -/
theorem lnq_strictMono (q : ℝ) (hq : 1 < q) {x y : ℝ} (hx : 0 < x) (hxy : x < y) :
    lnq q x < lnq q y := by
  unfold lnq
  have h : y ^ (1 - q) < x ^ (1 - q) := Real.rpow_lt_rpow_of_neg hx hxy (by linarith)
  have hc : 1 - q < 0 := by linarith
  rw [div_lt_div_right_of_neg hc]
  linarith

/-- `ln_q x → log x` for `q → 1`. -/
theorem lnq_tendsto_log (x : ℝ) (hx : 0 < x) :
    Tendsto (fun q => lnq q x) (𝓝[≠] 1) (𝓝 (Real.log x)) := by
  have hd : HasDerivAt (fun t : ℝ => x ^ t) (x ^ (0 : ℝ) * Real.log x) 0 :=
    (Real.hasStrictDerivAt_const_rpow hx 0).hasDerivAt
  have hs := hd.tendsto_slope_zero
  simp only [Real.rpow_zero, one_mul, zero_add, smul_eq_mul] at hs
  have hmap : Tendsto (fun q : ℝ => 1 - q) (𝓝[≠] 1) (𝓝[≠] 0) := by
    apply tendsto_nhdsWithin_of_tendsto_nhds_of_eventually_within
    · have : Tendsto (fun q : ℝ => 1 - q) (𝓝 1) (𝓝 (1 - 1)) :=
        tendsto_const_nhds.sub tendsto_id
      simpa using this.mono_left nhdsWithin_le_nhds
    · filter_upwards [self_mem_nhdsWithin] with q hq
      exact sub_ne_zero.mpr (Ne.symm hq)
  have := hs.comp hmap
  refine this.congr' ?_
  filter_upwards [self_mem_nhdsWithin] with q hq
  simp only [Function.comp, lnq]
  rw [div_eq_inv_mul]

/-! ## 3. The soft value of one site and its limits -/

/-- `v_α = α log (e^{a/α} + e^{b/α})`: soft value of deciding one site. -/
noncomputable def soft2 (α a b : ℝ) : ℝ := α * Real.log (Real.exp (a / α) + Real.exp (b / α))

theorem soft2_ge_max (α a b : ℝ) (hα : 0 < α) : max a b ≤ soft2 α a b := by
  unfold soft2
  have key : ∀ c, c ≤ max a b → Real.exp (c / α) ≤ Real.exp (a / α) + Real.exp (b / α) →
      c ≤ α * Real.log (Real.exp (a / α) + Real.exp (b / α)) := by
    intro c _ h
    have hl := Real.log_le_log (Real.exp_pos _) h
    rw [Real.log_exp] at hl
    have := mul_le_mul_of_nonneg_left hl hα.le
    rwa [mul_div_cancel₀ c hα.ne'] at this
  rcases le_total a b with h | h
  · rw [max_eq_right h]
    exact key b (le_max_right a b) (by linarith [Real.exp_pos (a / α)])
  · rw [max_eq_left h]
    exact key a (le_max_left a b) (by linarith [Real.exp_pos (b / α)])

theorem soft2_le_max_add (α a b : ℝ) (hα : 0 < α) :
    soft2 α a b ≤ max a b + α * Real.log 2 := by
  unfold soft2
  set m := max a b
  have ha : Real.exp (a / α) ≤ Real.exp (m / α) :=
    Real.exp_le_exp.mpr (div_le_div_of_nonneg_right (le_max_left a b) hα.le)
  have hb : Real.exp (b / α) ≤ Real.exp (m / α) :=
    Real.exp_le_exp.mpr (div_le_div_of_nonneg_right (le_max_right a b) hα.le)
  have hsum : Real.exp (a / α) + Real.exp (b / α) ≤ 2 * Real.exp (m / α) := by linarith
  have hpos : 0 < Real.exp (a / α) + Real.exp (b / α) := by positivity
  have hl := Real.log_le_log hpos hsum
  rw [Real.log_mul (by norm_num) (Real.exp_pos _).ne', Real.log_exp] at hl
  have := mul_le_mul_of_nonneg_left hl hα.le
  rw [mul_add, mul_div_cancel₀ m hα.ne'] at this
  linarith

/-- The hard limit: `v_α → max(a, b)` for `α → 0⁺`. -/
theorem soft2_tendsto_max (a b : ℝ) :
    Tendsto (fun α => soft2 α a b) (𝓝[>] 0) (𝓝 (max a b)) := by
  have hup : Tendsto (fun α : ℝ => max a b + α * Real.log 2) (𝓝[>] 0) (𝓝 (max a b)) := by
    have : Tendsto (fun α : ℝ => max a b + α * Real.log 2) (𝓝 0) (𝓝 (max a b + 0 * Real.log 2)) :=
      tendsto_const_nhds.add (tendsto_id.mul tendsto_const_nhds)
    simpa using this.mono_left nhdsWithin_le_nhds
  refine tendsto_of_tendsto_of_tendsto_of_le_of_le' tendsto_const_nhds hup ?_ ?_
  · filter_upwards [self_mem_nhdsWithin] with α hα using soft2_ge_max α a b hα
  · filter_upwards [self_mem_nhdsWithin] with α hα using soft2_le_max_add α a b hα

/-- SAC identity for the sign: `α log π_α(s) = r^s - v_α`. -/
theorem sac_identity (α a b : ℝ) (hα : 0 < α) :
    α * Real.log (Real.exp (a / α) / (Real.exp (a / α) + Real.exp (b / α))) = a - soft2 α a b := by
  unfold soft2
  have hpos : (0 : ℝ) < Real.exp (a / α) + Real.exp (b / α) := by positivity
  rw [Real.log_div (Real.exp_pos _).ne' hpos.ne', Real.log_exp, mul_sub,
    mul_div_cancel₀ a hα.ne']

/-- A site that is free in every cluster (`r^+ = r^- = 0`) has `v_α = α log 2`. -/
theorem soft2_unfrozen (α : ℝ) : soft2 α 0 0 = α * Real.log 2 := by
  unfold soft2
  norm_num [one_add_one_eq_two]

/-- A site frozen to one value (`r^s = 0`, `r^{-s} = b → -∞`) has `0 ≤ v_α ≤ α e^{b/α}`. -/
theorem soft2_frozen (α b : ℝ) (hα : 0 < α) :
    0 ≤ soft2 α 0 b ∧ soft2 α 0 b ≤ α * Real.exp (b / α) := by
  unfold soft2
  simp only [zero_div, Real.exp_zero]
  constructor
  · apply mul_nonneg hα.le
    apply Real.log_nonneg
    linarith [Real.exp_pos (b / α)]
  · apply mul_le_mul_of_nonneg_left _ hα.le
    have := Real.log_le_sub_one_of_pos (show 0 < 1 + Real.exp (b / α) by positivity)
    linarith

/-! ## 4. SAC value as a thermodynamic sum over actions -/

/-- Soft Bellman value `α log Σ_a e^{Q_a/α}`. -/
noncomputable def soft {ι : Type*} [Fintype ι] (α : ℝ) (Q : ι → ℝ) : ℝ :=
  α * Real.log (∑ i, Real.exp (Q i / α))

/-- Gibbs variational principle (Prop. `prop:shannon` in max-plus form):
for every policy `p`, the SAC objective `E_p[Q] + α H(p)` is at most the soft value. -/
theorem sac_objective_le_soft {ι : Type*} [Fintype ι] [Nonempty ι] (α : ℝ) (hα : 0 < α)
    (Q p : ι → ℝ) (hp : ∀ i, 0 ≤ p i) (hs : ∑ i, p i = 1) :
    ∑ i, p i * Q i - α * ∑ i, p i * Real.log (p i) ≤ soft α Q := by
  unfold soft
  set Z := ∑ i, Real.exp (Q i / α)
  have hZ : 0 < Z := Finset.sum_pos (fun i _ => Real.exp_pos _) Finset.univ_nonempty
  set g : ι → ℝ := fun i => Real.exp (Q i / α) / Z
  have hg : ∀ i, 0 < g i := fun i => div_pos (Real.exp_pos _) hZ
  have hgs : ∑ i, g i = 1 := by
    simp only [g, ← Finset.sum_div]
    exact div_self hZ.ne'
  have hQ : ∀ i, Q i = α * Real.log (g i) + α * Real.log Z := by
    intro i
    simp only [g]
    rw [Real.log_div (Real.exp_pos _).ne' hZ.ne', Real.log_exp]
    field_simp
    ring
  -- termwise Gibbs inequality: p (log g - log p) ≤ g - p
  have hterm : ∀ i, p i * (Real.log (g i) - Real.log (p i)) ≤ g i - p i := by
    intro i
    rcases (hp i).eq_or_lt with h0 | hpos
    · rw [← h0]; simp; exact (hg i).le
    · have hl := Real.log_le_sub_one_of_pos (div_pos (hg i) hpos)
      rw [Real.log_div (hg i).ne' hpos.ne'] at hl
      have h1 := mul_le_mul_of_nonneg_left hl hpos.le
      have e : p i * (g i / p i - 1) = g i - p i := by field_simp
      linarith
  have hsum : ∑ i, p i * (Real.log (g i) - Real.log (p i)) ≤ 0 := by
    calc ∑ i, p i * (Real.log (g i) - Real.log (p i)) ≤ ∑ i, (g i - p i) :=
          Finset.sum_le_sum (fun i _ => hterm i)
      _ = 0 := by rw [Finset.sum_sub_distrib, hgs, hs, sub_self]
  have h1 : ∑ i, p i * Q i = α * ∑ i, p i * Real.log (g i) + α * Real.log Z := by
    calc ∑ i, p i * Q i = ∑ i, (α * (p i * Real.log (g i)) + α * Real.log Z * p i) := by
          apply Finset.sum_congr rfl
          intro i _
          rw [hQ i]
          ring
      _ = α * ∑ i, p i * Real.log (g i) + α * Real.log Z * ∑ i, p i := by
          rw [Finset.sum_add_distrib, ← Finset.mul_sum, ← Finset.mul_sum]
      _ = α * ∑ i, p i * Real.log (g i) + α * Real.log Z := by rw [hs, mul_one]
  have h2 : ∑ i, p i * (Real.log (g i) - Real.log (p i))
      = ∑ i, p i * Real.log (g i) - ∑ i, p i * Real.log (p i) := by
    rw [← Finset.sum_sub_distrib]
    apply Finset.sum_congr rfl
    intro i _
    ring
  have hrewrite : ∑ i, p i * Q i - α * ∑ i, p i * Real.log (p i)
      = α * Real.log Z + α * ∑ i, p i * (Real.log (g i) - Real.log (p i)) := by
    rw [h1, h2]
    ring
  rw [hrewrite]
  nlinarith [mul_nonpos_of_nonneg_of_nonpos hα.le hsum]

/-- The bound is attained by the Gibbs (SAC) policy `p ∝ e^{Q/α}`. -/
theorem sac_objective_gibbs {ι : Type*} [Fintype ι] [Nonempty ι] (α : ℝ) (hα : 0 < α)
    (Q : ι → ℝ) :
    let Z := ∑ i, Real.exp (Q i / α)
    let p := fun i => Real.exp (Q i / α) / Z
    ∑ i, p i * Q i - α * ∑ i, p i * Real.log (p i) = soft α Q := by
  intro Z p
  unfold soft
  have hZ : 0 < Z := Finset.sum_pos (fun i _ => Real.exp_pos _) Finset.univ_nonempty
  have hlog : ∀ i, Real.log (p i) = Q i / α - Real.log Z := by
    intro i
    simp only [p]
    rw [Real.log_div (Real.exp_pos _).ne' hZ.ne', Real.log_exp]
  have hps : ∑ i, p i = 1 := by
    simp only [p, ← Finset.sum_div]
    exact div_self hZ.ne'
  have h1 : ∑ i, p i * Real.log (p i) = (∑ i, p i * Q i) / α - Real.log Z := by
    calc ∑ i, p i * Real.log (p i) = ∑ i, (p i * Q i / α - Real.log Z * p i) := by
          apply Finset.sum_congr rfl
          intro i _
          rw [hlog i]
          ring
      _ = (∑ i, p i * Q i) / α - Real.log Z * ∑ i, p i := by
          rw [Finset.sum_sub_distrib, Finset.sum_div, Finset.mul_sum]
      _ = (∑ i, p i * Q i) / α - Real.log Z := by rw [hps, mul_one]
  rw [h1]
  field_simp
  ring

/-! ## 5. The frozen-field value factorizes over sites -/

/-- With frozen fields, the total return of a complete assignment does not depend
on the order in which the sites are fixed. -/
theorem frozen_order_free {ι : Type*} [Fintype ι] (r : ι → Bool → ℝ) (t : ι → Bool)
    (σ : Equiv.Perm ι) :
    ∑ i, r (σ i) (t (σ i)) = ∑ i, r i (t i) :=
  Equiv.sum_comp σ (fun i => r i (t i))

/-- The partition function over complete sign assignments factorizes over sites,
so the soft value of the frozen-field future is the sum of the site values. -/
theorem frozen_soft_value {ι : Type*} [Fintype ι] [DecidableEq ι] (α : ℝ)
    (r : ι → Bool → ℝ) :
    α * Real.log (∑ t : ι → Bool, Real.exp ((∑ i, r i (t i)) / α))
      = ∑ i, soft2 α (r i true) (r i false) := by
  have hfac : ∑ t : ι → Bool, Real.exp ((∑ i, r i (t i)) / α)
      = ∏ i, (Real.exp (r i true / α) + Real.exp (r i false / α)) := by
    have : ∀ t : ι → Bool, Real.exp ((∑ i, r i (t i)) / α) = ∏ i, Real.exp (r i (t i) / α) := by
      intro t
      rw [Finset.sum_div, Real.exp_sum]
    simp only [this]
    rw [← Fintype.prod_sum (fun i (s : Bool) => Real.exp (r i s / α))]
    apply Finset.prod_congr rfl
    intro i _
    rw [Fintype.sum_bool]
  rw [hfac, Real.log_prod (fun i _ => by positivity), Finset.mul_sum]
  rfl

/-! ## 6. Decomposition of the score -/

/-- `Q - V = (r_k^s - v_k) + D`, where `D` is the change of the other site values. -/
theorem score_decomposition {ι : Type*} [DecidableEq ι] (L L' : Finset ι) (k : ι) (hk : k ∈ L)
    (v v' : ι → ℝ) (r : ℝ) :
    r + ∑ l ∈ L', v' l - ∑ l ∈ L, v l
      = (r - v k) + (∑ l ∈ L', v' l - ∑ l ∈ L.erase k, v l) := by
  rw [← Finset.add_sum_erase L v hk]
  ring

/-! ## 7. Why the literal SAC two-step fails -/

/-- Hard limit: if some site has bias `1`, the best next reward `max_l log b_l` is `0`,
whatever the first move was. -/
theorem hard_two_step_degenerate {ι : Type*} (s : Finset ι) (b : ι → ℝ) (k : ι) (hk : k ∈ s)
    (hb0 : ∀ l ∈ s, 0 ≤ b l) (hb1 : ∀ l ∈ s, b l ≤ 1) (hbk : b k = 1) :
    s.sup' ⟨k, hk⟩ (fun l => Real.log (b l)) = 0 := by
  apply le_antisymm
  · exact Finset.sup'_le _ _ (fun l hl => Real.log_nonpos (hb0 l hl) (hb1 l hl))
  · have := Finset.le_sup' (fun l => Real.log (b l)) hk
    simpa [hbk] using this

/-- Soft limit: a local change `δ` of a partition function `Z` moves `α log Z`
by at most `α δ / Z`, which is of order `1/N` when `Z` sums `2N` terms of order one. -/
theorem dilution (α Z δ : ℝ) (hα : 0 ≤ α) (hZ : 0 < Z) (hZδ : 0 < Z + δ) :
    α * δ / (Z + δ) ≤ α * Real.log (Z + δ) - α * Real.log Z ∧
      α * Real.log (Z + δ) - α * Real.log Z ≤ α * δ / Z := by
  have hdiff : Real.log (Z + δ) - Real.log Z = Real.log ((Z + δ) / Z) :=
    (Real.log_div hZδ.ne' hZ.ne').symm
  have hup : Real.log ((Z + δ) / Z) ≤ δ / Z := by
    have := Real.log_le_sub_one_of_pos (div_pos hZδ hZ)
    have e : (Z + δ) / Z - 1 = δ / Z := by field_simp; ring
    linarith
  have hlo : δ / (Z + δ) ≤ Real.log ((Z + δ) / Z) := by
    have := Real.log_le_sub_one_of_pos (div_pos hZ hZδ)
    rw [Real.log_div hZ.ne' hZδ.ne'] at this
    rw [Real.log_div hZδ.ne' hZ.ne']
    have e : Z / (Z + δ) - 1 = -(δ / (Z + δ)) := by field_simp; ring
    linarith
  constructor
  · rw [← mul_sub, hdiff, mul_div_assoc]
    exact mul_le_mul_of_nonneg_left hlo hα
  · rw [← mul_sub, hdiff, mul_div_assoc]
    exact mul_le_mul_of_nonneg_left hup hα

end SoftQ
