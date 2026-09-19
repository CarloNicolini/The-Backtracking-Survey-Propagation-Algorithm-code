import numpy as np
import pytest

from sat_instance import (
    ContradictionError,
    FactorGraph,
    SATFormula,
    clause_count,
    dumps_dimacs,
    generate_random_ksat,
    loads_dimacs,
)


def test_clause_count_matches_cpp_ceil():
    assert clause_count(10, 4.25) == 43
    assert clause_count(100, 4.0) == 400
    assert clause_count(3, 4.25) == 13


def test_generated_clauses_have_k_distinct_vars():
    f = generate_random_ksat(20, 3.0, k=3, seed=1)
    assert f.m == clause_count(20, 3.0)
    for a in range(f.m):
        vs = [int(v) for v in f.clauses[a] if v >= 0]
        assert len(vs) == 3
        assert len(set(vs)) == 3
        assert set(f.signs[a]).issubset({-1, 1})


def test_dimacs_roundtrip():
    f = generate_random_ksat(12, 2.5, k=3, seed=7)
    text = dumps_dimacs(f)
    g = loads_dimacs(text)
    assert g.n == f.n
    assert g.m == f.m
    assert np.array_equal(g.clauses, f.clauses)
    assert np.array_equal(g.signs, f.signs)
    assert g.seed == 7


def test_freeze_satisfying_literal_drops_clause():
    clauses = np.array([[0, 1, 2]], dtype=np.int32)
    signs = np.array([[1, 1, 1]], dtype=np.int8)
    formula = SATFormula(n=3, k=3, alpha=1 / 3, clauses=clauses, signs=signs)
    g = FactorGraph(formula, rng=np.random.default_rng(0))
    g.sC[:] = 0.5
    g.freeze(0, True)
    assert not g.unsat_mask[0]
    assert g.clause_sat[0]
    assert g.fixed[0]
    assert g.n_t == 2
    assert g.m_t == 0


def test_freeze_unsat_literal_shortens_clause():
    clauses = np.array([[0, 1, 2]], dtype=np.int32)
    signs = np.array([[1, 1, 1]], dtype=np.int8)
    formula = SATFormula(n=3, k=3, alpha=1 / 3, clauses=clauses, signs=signs)
    g = FactorGraph(formula, rng=np.random.default_rng(0))
    g.freeze(0, False)  # x0=False does not satisfy (x0 v x1 v x2)
    assert g.unsat_mask[0]
    assert g.clause_len[0] == 2
    assert not g.go_forward[0, 0]


def test_empty_unsat_is_contradiction():
    clauses = np.array([[0], [0]], dtype=np.int32)
    signs = np.array([[1], [-1]], dtype=np.int8)
    formula = SATFormula(n=1, k=1, alpha=2.0, clauses=clauses, signs=signs)
    g = FactorGraph(formula, rng=np.random.default_rng(0))
    with pytest.raises(ContradictionError):
        g.unit_propagate()


def test_unit_propagation_fixes_singleton():
    clauses = np.array([[0]], dtype=np.int32)
    signs = np.array([[1]], dtype=np.int8)
    formula = SATFormula(n=1, k=1, alpha=1.0, clauses=clauses, signs=signs)
    g = FactorGraph(formula, rng=np.random.default_rng(0))
    g.unit_propagate()
    assert g.fixed[0]
    assert bool(g.value[0]) is True
    assert bool(g.forced_by_up[0]) is True


def test_unfreeze_restores_unsat_clause():
    clauses = np.array([[0, 1, 2]], dtype=np.int32)
    signs = np.array([[1, 1, 1]], dtype=np.int8)
    formula = SATFormula(n=3, k=3, alpha=1 / 3, clauses=clauses, signs=signs)
    g = FactorGraph(formula, rng=np.random.default_rng(0))
    g.freeze(0, True)
    assert g.m_t == 0
    g.unfreeze(0)
    assert not g.fixed[0]
    assert g.unsat_mask[0]
    assert g.m_t == 1
    assert g.clause_len[0] == 3
