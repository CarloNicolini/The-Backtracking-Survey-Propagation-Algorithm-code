"""Smoke-test Markovianity helpers without a full diagnostic run."""

from pathlib import Path
import sys

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "experiments"))

from markovianity import jaccard_disagreement, match_pairs, Snapshot  # noqa: E402
from sat_instance import FactorGraph, generate_random_ksat  # noqa: E402
from survey_propagation import iterate_sp, state_features  # noqa: E402


def test_jaccard_and_match_logic():
    assert jaccard_disagreement(frozenset({1, 2}), frozenset({1, 2})) == 0.0
    assert jaccard_disagreement(frozenset({1}), frozenset({2})) == 1.0

    formula = generate_random_ksat(8, 2.0, k=3, seed=1)
    g = FactorGraph(formula, rng=np.random.default_rng(0))
    iterate_sp(g)
    phi = state_features(g)
    s = Snapshot(
        instance_id=0,
        history_id=0,
        step=0,
        phi=phi,
        L=g.n_t,
        sigma=g.complexity,
        frozen=frozenset(),
        i_stale=g.sC.copy(),
        graph=g,
    )
    s2 = Snapshot(
        instance_id=0,
        history_id=1,
        step=0,
        phi=phi + 0.01,
        L=g.n_t,
        sigma=g.complexity,
        frozen=frozenset({0, 1, 2}),
        i_stale=g.sC.copy(),
        graph=g,
    )
    pairs = match_pairs([s, s2], phi_tol=1.0, min_jaccard=0.1, l_bin=2)
    assert len(pairs) == 1
