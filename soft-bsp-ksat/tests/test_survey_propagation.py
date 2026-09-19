import numpy as np

from sat_instance import FactorGraph, SATFormula, generate_random_ksat
from survey_propagation import (
    SPConfig,
    compute_surveys,
    iterate_sp,
    make_products,
    polarization_f,
    state_features,
)


def _zero_messages(graph: FactorGraph) -> None:
    graph.u[:] = 0.0
    graph.update[:] = 0.0
    graph.old_s[:] = 0.0
    graph.div_s[:] = 1.0


def test_all_zero_messages_are_paramagnetic_fixed_point():
    formula = generate_random_ksat(8, 2.0, k=3, seed=2)
    g = FactorGraph(formula, rng=np.random.default_rng(0))
    _zero_messages(g)
    eta = iterate_sp(g, SPConfig(t_max=32, epsilon=1e-8))
    assert eta == 0
    np.testing.assert_allclose(g.u, 0.0, atol=1e-12)
    unfixed = ~g.fixed
    np.testing.assert_allclose(g.sI[unfixed], 1.0, atol=1e-9)
    np.testing.assert_allclose(g.sT[unfixed], 0.0, atol=1e-9)
    np.testing.assert_allclose(g.sF[unfixed], 0.0, atol=1e-9)
    assert abs(g.complexity) < 1e-9
    assert polarization_f(g) < 1e-9


def test_empty_formula_sigma_zero():
    formula = SATFormula(
        n=4,
        k=1,
        alpha=0.0,
        clauses=np.zeros((0, 1), dtype=np.int32),
        signs=np.zeros((0, 1), dtype=np.int8),
    )
    g = FactorGraph(formula, rng=np.random.default_rng(0))
    iterate_sp(g, SPConfig(t_max=8))
    assert g.complexity == 0.0
    assert g.n_t == 4


def test_single_clause_closed_form_message_stays_zero_from_zero():
    """Unconstrained cavity products are 1, so u_new = 0 from u=0."""
    clauses = np.array([[0, 1, 2]], dtype=np.int32)
    signs = np.array([[1, 1, 1]], dtype=np.int8)
    formula = SATFormula(n=3, k=3, alpha=1 / 3, clauses=clauses, signs=signs)
    g = FactorGraph(formula, rng=np.random.default_rng(0))
    _zero_messages(g)
    iterate_sp(g, SPConfig(t_max=16, epsilon=1e-12))
    np.testing.assert_allclose(g.u[0, :3], 0.0, atol=1e-12)
    make_products(g)
    compute_surveys(g)
    assert abs(g.sI[0] - 1.0) < 1e-9


def test_state_features_finite():
    formula = generate_random_ksat(10, 2.0, k=3, seed=3)
    g = FactorGraph(formula, rng=np.random.default_rng(1))
    iterate_sp(g)
    phi = state_features(g)
    assert phi.shape[0] >= 10
    assert np.all(np.isfinite(phi))
