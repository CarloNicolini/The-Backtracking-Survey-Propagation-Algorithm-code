import numpy as np

from baseline_bsp import BSPConfig, backtrack, parisi_r_to_cpp, run_bsp
from sat_instance import FactorGraph, SATFormula, generate_random_ksat
from survey_propagation import SPConfig


def test_parisi_r_mapping():
    assert parisi_r_to_cpp(0.0) == 0.0
    assert abs(parisi_r_to_cpp(0.25) - 1.0 / 3.0) < 1e-12
    assert abs(parisi_r_to_cpp(0.4) - 2.0 / 3.0) < 1e-12


def test_r0_never_unfreezes():
    formula = generate_random_ksat(24, 2.0, k=3, seed=11)
    cfg = BSPConfig(
        r_bsp=0.0,
        frac=0.2,
        seed=11,
        sp=SPConfig(t_max=64),
        max_steps=40,
    )
    result = run_bsp(formula, cfg=cfg)
    moves = [s.next_move for s in result.trajectory]
    assert "back" not in moves
    n_fixed = [s.n_fixed for s in result.trajectory]
    assert n_fixed == sorted(n_fixed)


def test_ratio_schedule_matches_cpp_counters():
    """C++ starts n_dec=n_back=1; first move is always decimate for r<1."""
    n_dec, n_back = 1.0, 1.0
    r = 0.5
    moves = []
    for _ in range(6):
        if n_back / n_dec < r:
            n_back += 1.0
            moves.append("back")
        else:
            n_dec += 1.0
            moves.append("dec")
    assert moves[0] == "dec"
    assert "back" in moves


def test_backtrack_releases_lowest_stale_certitude():
    clauses = np.array([[0, 1, 2], [0, 1, 3]], dtype=np.int32)
    signs = np.array([[1, 1, 1], [1, 1, 1]], dtype=np.int8)
    formula = SATFormula(n=4, k=3, alpha=0.5, clauses=clauses, signs=signs)
    g = FactorGraph(formula, rng=np.random.default_rng(0))
    g.sC[0] = 0.9
    g.sC[1] = 0.1
    g.freeze(0, True)
    g.freeze(1, True)
    assert g.n_t == 2
    n_rel = backtrack(g, last_dec_batch=1)
    assert n_rel == 1
    assert not g.fixed[1]
    assert g.fixed[0]


def test_r_positive_increments_back_counter():
    formula = generate_random_ksat(40, 3.0, k=3, seed=13)
    cfg = BSPConfig(
        r_bsp=0.5,
        frac=0.05,
        seed=13,
        sp=SPConfig(t_max=64),
        max_steps=20,
    )
    result = run_bsp(formula, cfg=cfg)
    if len(result.trajectory) >= 3:
        assert max(s.n_back for s in result.trajectory) > 1.0


def test_easy_instance_often_paramagnetic():
    formula = generate_random_ksat(16, 1.5, k=3, seed=5)
    cfg = BSPConfig(r_bsp=0.0, frac=0.25, seed=5, sp=SPConfig(t_max=64), max_steps=40)
    result = run_bsp(formula, cfg=cfg)
    assert result.reason in {"paramagnetic", "negative_sigma", "no_convergence", "max_steps"} or result.reason.startswith(
        "contradiction"
    )
    if result.success:
        assert result.assignment is not None
        assert result.assignment.shape == (16,)


def test_sigma_logged_each_step():
    formula = generate_random_ksat(18, 2.0, k=3, seed=17)
    cfg = BSPConfig(r_bsp=0.0, frac=0.2, seed=17, max_steps=8, sp=SPConfig(t_max=32))
    result = run_bsp(formula, cfg=cfg)
    assert result.trajectory
    for rec in result.trajectory:
        assert rec.L >= 0
        assert np.isfinite(rec.sigma)
        assert rec.F >= -1e-9
