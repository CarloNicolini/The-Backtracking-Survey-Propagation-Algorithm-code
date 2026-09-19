"""Survey propagation fixed point, surveys, complexity, P(i), I(k), F(L).

Equations follow Graph.cpp / Vertex.cpp (ρ=1 SP, CERT scorer). This module
is the single source of truth for Σ; later RL code must call it rather than
approximate complexity.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np
from numba import njit

from sat_instance import FactorGraph, SPNoConvergence, ContradictionError

ZERO = 1.0e-10
EPSILON = 0.01
T_MAX = 1024
RHO_SP = 1.0


@dataclass
class SPConfig:
    t_max: int = T_MAX
    epsilon: float = EPSILON
    zero: float = ZERO
    rho_sp: float = RHO_SP
    damping: float = 0.0


@njit(cache=True)
def _make_products(
    prod_plus: np.ndarray,
    prod_minus: np.ndarray,
    u: np.ndarray,
    plus_ptr: np.ndarray,
    plus_a: np.ndarray,
    plus_p: np.ndarray,
    minus_ptr: np.ndarray,
    minus_a: np.ndarray,
    minus_p: np.ndarray,
    fixed: np.ndarray,
) -> None:
    n = prod_plus.shape[0]
    for i in range(n):
        if fixed[i]:
            continue
        acc = 1.0
        start = plus_ptr[i]
        end = plus_ptr[i + 1]
        for e in range(start, end):
            acc *= 1.0 - u[plus_a[e], plus_p[e]]
        prod_plus[i] = acc
        acc = 1.0
        start = minus_ptr[i]
        end = minus_ptr[i + 1]
        for e in range(start, end):
            acc *= 1.0 - u[minus_a[e], minus_p[e]]
        prod_minus[i] = acc


@njit(cache=True)
def _sp_sweep(
    u: np.ndarray,
    go_forward: np.ndarray,
    clause_vars: np.ndarray,
    clause_signs: np.ndarray,
    clause_len_init: np.ndarray,
    unsat_mask: np.ndarray,
    prod_plus: np.ndarray,
    prod_minus: np.ndarray,
    update: np.ndarray,
    old_s: np.ndarray,
    div_s: np.ndarray,
    rho: float,
    zero: float,
    damping: float,
    eps: float,
) -> tuple:
    """One Jacobi-products sweep. Returns (n_unconv, hit_one_clause_or_-1)."""
    m = u.shape[0]
    n_unconv = 0
    hit_one = -1
    for c in range(m):
        if not unsat_mask[c]:
            continue
        s = clause_len_init[c]
        for i in range(s):
            if not go_forward[c, i]:
                continue
            new = 1.0
            norm = 1.0
            for l in range(s):
                if i == l or not go_forward[c, l]:
                    continue
                v = clause_vars[c, l]
                ds = div_s[c, l]
                if clause_signs[c, l] > 0:
                    prod_s = prod_plus[v] * ds
                    prod_u = prod_minus[v]
                else:
                    prod_s = prod_minus[v] * ds
                    prod_u = prod_plus[v]
                new *= (1.0 - prod_u) * prod_s
                norm *= prod_s + prod_u - rho * prod_s * prod_u
            if norm == 0.0:
                u_new = 0.0
            else:
                u_new = new / norm
            if u_new < zero:
                u_new = 0.0
            if u_new == 1.0:
                return n_unconv, c
            old_s[c, i] = update[c, i]
            update[c, i] = u_new + (old_s[c, i] - u_new) * damping
            if n_unconv == 0 and abs(update[c, i] - old_s[c, i]) > eps:
                n_unconv = 1
        for i in range(s):
            u[c, i] = update[c, i]
            om = 1.0 - update[c, i]
            if om == 0.0:
                div_s[c, i] = 1.0e300
            else:
                div_s[c, i] = 1.0 / om
    return n_unconv, hit_one


@njit(cache=True)
def _clause_complexity(
    u: np.ndarray,
    go_forward: np.ndarray,
    clause_vars: np.ndarray,
    clause_signs: np.ndarray,
    clause_len_init: np.ndarray,
    unsat_mask: np.ndarray,
    prod_plus: np.ndarray,
    prod_minus: np.ndarray,
    div_s: np.ndarray,
    rho: float,
) -> float:
    m = u.shape[0]
    acc = 0.0
    for c in range(m):
        if not unsat_mask[c]:
            continue
        ps = 1.0
        pu = 1.0
        s = clause_len_init[c]
        for j in range(s):
            if not go_forward[c, j]:
                continue
            v = clause_vars[c, j]
            ds = div_s[c, j]
            if clause_signs[c, j] > 0:
                prod_s = prod_plus[v] * ds
                prod_u = prod_minus[v]
            else:
                prod_s = prod_minus[v] * ds
                prod_u = prod_plus[v]
            ps *= prod_s + prod_u - rho * prod_s * prod_u
            pu *= (1.0 - prod_u) * prod_s
        diff = ps - pu
        if diff <= 0.0:
            diff = 1.0e-300
        acc += np.log(diff)
    return acc


def make_products(graph: FactorGraph) -> None:
    _make_products(
        graph.prod_plus,
        graph.prod_minus,
        graph.u,
        graph.plus_ptr,
        graph.plus_a,
        graph.plus_p,
        graph.minus_ptr,
        graph.minus_a,
        graph.minus_p,
        graph.fixed,
    )


def compute_surveys(graph: FactorGraph) -> None:
    """sT, sF, sI, certitude P(i)=1-min(sT,sF), and variable complexity term."""
    n = graph.n
    graph.complexity_variables = 0.0
    for i in range(n):
        if graph.fixed[i]:
            continue
        pp = graph.prod_plus[i]
        pm = graph.prod_minus[i]
        p_plus = (1.0 - pp) * pm
        p_minus = (1.0 - pm) * pp
        p_i = pp * pm
        z = p_plus + p_minus + p_i
        graph.var_complexity[i] = z
        if z <= 0.0:
            graph.sT[i] = 0.0
            graph.sF[i] = 0.0
            graph.sI[i] = 1.0
        else:
            graph.sT[i] = p_plus / z
            graph.sF[i] = p_minus / z
            graph.sI[i] = 1.0 - graph.sT[i] - graph.sF[i]
        graph.sC[i] = 1.0 - min(graph.sT[i], graph.sF[i])
        graph.complexity_variables += (float(graph.degree[i]) - 1.0) * np.log(max(z, 1.0e-300))
    if graph.complexity_variables == 0.0:
        graph.complexity = 0.0
    else:
        graph.complexity = graph.complexity_clauses - graph.complexity_variables


def polarization_f(graph: FactorGraph) -> float:
    """Parisi F(L) = mean |sT-sF| over the L unassigned variables."""
    l = graph.n_t
    if l <= 0:
        return 0.0
    mask = ~graph.fixed
    return float(np.sum(np.abs(graph.sT[mask] - graph.sF[mask])) / l)


def certitude(sT: np.ndarray, sF: np.ndarray) -> np.ndarray:
    """P(i) / I(k) CERT scorer: 1 - min(sT, sF)."""
    return 1.0 - np.minimum(sT, sF)


def iterate_sp(graph: FactorGraph, cfg: SPConfig | None = None) -> int:
    """Run SP to a fixed point on the residual graph. Warm-starts from current u.

    Returns the number of sweeps (eta). Raises ContradictionError or SPNoConvergence.
    """
    if cfg is None:
        cfg = SPConfig()
    restarts = 0
    while True:
        graph.unit_propagate()
        make_products(graph)
        conv = False
        eta = 0
        for t in range(cfg.t_max):
            eta = t
            n_unconv, hit_one = _sp_sweep(
                graph.u,
                graph.go_forward,
                graph.clauses,
                graph.signs,
                graph.clause_len_init,
                graph.unsat_mask,
                graph.prod_plus,
                graph.prod_minus,
                graph.update,
                graph.old_s,
                graph.div_s,
                cfg.rho_sp,
                cfg.zero,
                cfg.damping,
                cfg.epsilon,
            )
            if hit_one >= 0:
                graph.freeze_to_satisfy_clause(int(hit_one))
                restarts += 1
                if restarts > graph.n + 2:
                    raise ContradictionError("repeated survey=1 unit propagation")
                break
            make_products(graph)
            if n_unconv == 0:
                graph.complexity_clauses = float(
                    _clause_complexity(
                        graph.u,
                        graph.go_forward,
                        graph.clauses,
                        graph.signs,
                        graph.clause_len_init,
                        graph.unsat_mask,
                        graph.prod_plus,
                        graph.prod_minus,
                        graph.div_s,
                        cfg.rho_sp,
                    )
                )
                conv = True
                graph.eta = t
                break
        else:
            raise SPNoConvergence(f"no SP fixed point in {cfg.t_max} iterations")
        if conv:
            compute_surveys(graph)
            return graph.eta


def state_features(graph: FactorGraph, n_hist: int = 8) -> np.ndarray:
    """Permutation-invariant φ used by the Markovianity diagnostic (not the full graph).

    Layout: [L/N, Σ/N, mean/std of sT,sF,sI,P on unassigned, histogram of P].
    """
    n = graph.n
    l = graph.n_t
    l_frac = l / float(n) if n else 0.0
    sig_frac = graph.complexity / float(n) if n else 0.0
    mask = ~graph.fixed
    if l == 0 or not np.any(mask):
        stats = np.zeros(8, dtype=np.float64)
        hist = np.zeros(n_hist, dtype=np.float64)
    else:
        sT = graph.sT[mask]
        sF = graph.sF[mask]
        sI = graph.sI[mask]
        p = certitude(sT, sF)
        stats = np.array(
            [
                float(sT.mean()), float(sT.std()),
                float(sF.mean()), float(sF.std()),
                float(sI.mean()), float(sI.std()),
                float(p.mean()), float(p.std()),
            ],
            dtype=np.float64,
        )
        hist, _ = np.histogram(p, bins=n_hist, range=(0.0, 1.0), density=True)
        hist = hist.astype(np.float64)
    return np.concatenate(([l_frac, sig_frac], stats, hist))
