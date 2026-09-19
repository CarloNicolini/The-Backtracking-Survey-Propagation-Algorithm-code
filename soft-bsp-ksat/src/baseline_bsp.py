"""Original fixed-ratio-r SID / BSP loop (Nature/C++ convention).

r_cpp = n_back / n_dec, with both counters starting at 1 (Header.h / Graph.cpp).
Parisi 2003 r_p = n_back / (n_back+n_dec) maps by r_cpp = r_p / (1-r_p).

Backtracking ranks frozen variables by stale certitude at freeze time, matching
C++ surveys() which does not recompute I(k) on assigned variables.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Callable

import numpy as np

from sat_instance import (
    BSPError,
    ContradictionError,
    FactorGraph,
    NegativeComplexityError,
    SATFormula,
    SPNoConvergence,
    generate_random_ksat,
    load_dimacs,
)
from survey_propagation import SPConfig, iterate_sp, polarization_f
from walksat import walksat


def parisi_r_to_cpp(r_p: float) -> float:
    """Map Parisi 2003 r (back / total, r<0.5) to C++ / Nature r (back / dec, r<1)."""
    if r_p < 0.0:
        raise ValueError(f"Parisi r must be >= 0, got {r_p}")
    if r_p >= 0.5:
        raise ValueError(f"Parisi r must be < 0.5, got {r_p}")
    if r_p == 0.0:
        return 0.0
    return r_p / (1.0 - r_p)


def cpp_r_to_parisi(r_cpp: float) -> float:
    if r_cpp < 0.0 or r_cpp >= 1.0:
        raise ValueError(f"C++ r must be in [0, 1), got {r_cpp}")
    return r_cpp / (1.0 + r_cpp)


@dataclass
class BSPConfig:
    r_bsp: float = 0.9
    frac: float = 0.00125
    negative_sigma: float = -2.0
    sp: SPConfig = field(default_factory=SPConfig)
    walksat_flips: int = 100_000
    seed: int | None = None
    noisy_topk: int | None = None
    max_steps: int | None = None


@dataclass
class StepRecord:
    step: int
    L: int
    M_t: int
    sigma: float
    sigma_per_n: float
    F: float
    eta: int
    next_move: str
    n_fixed: int
    unit_prop: int
    n_dec: float
    n_back: float


@dataclass
class BSPResult:
    success: bool
    reason: str
    trajectory: list[StepRecord]
    assignment: np.ndarray | None
    graph: FactorGraph


OnStep = Callable[[FactorGraph, StepRecord], None]


def _decimation_batch_size(n_t: int, frac: float) -> int:
    batch = int(frac * float(n_t))
    if batch < 1:
        return 1
    return batch


def _choose_decimation_vars(
    graph: FactorGraph,
    batch: int,
    rng: np.random.Generator,
    noisy_topk: int | None,
) -> np.ndarray:
    unfixed = np.flatnonzero(~graph.fixed)
    if unfixed.size == 0:
        return unfixed
    order = np.argsort(-graph.sC[unfixed], kind="mergesort")
    ranked = unfixed[order]
    if noisy_topk is not None and noisy_topk > 1:
        k = min(max(int(noisy_topk), batch), ranked.size)
        pool = ranked[:k]
        if pool.size <= batch:
            return pool
        pick = rng.choice(pool, size=batch, replace=False)
        return pick
    return ranked[: min(batch, ranked.size)]


def decimate(graph: FactorGraph, cfg: BSPConfig, rng: np.random.Generator) -> int:
    graph.unit_prop_count = 0
    batch = _decimation_batch_size(graph.n_t, cfg.frac)
    chosen = _choose_decimation_vars(graph, batch, rng, cfg.noisy_topk)
    for v in chosen:
        v = int(v)
        value = bool(graph.sT[v] > graph.sF[v])
        graph.freeze(v, value, forced_by_up=False)
    return int(chosen.size)


def backtrack(graph: FactorGraph, last_dec_batch: int) -> int:
    size = last_dec_batch + graph.unit_prop_count
    graph.unit_prop_count = 0
    n_fixed = len(graph.fixed_list)
    if n_fixed == 0 or size <= 0:
        return 0
    size = min(size, n_fixed)
    frozen = np.array(graph.fixed_list, dtype=np.int32)
    # Stale certitude: surveys() does not update sC on fixed vars.
    order = np.argsort(-graph.sC[frozen], kind="mergesort")
    ranked = frozen[order]
    released = 0
    for v in ranked[::-1]:
        v = int(v)
        if graph.forced_by_up[v]:
            break
        graph.unfreeze(v)
        released += 1
        if released >= size:
            break
    return released


def _try_walksat(graph: FactorGraph, cfg: BSPConfig, rng: np.random.Generator) -> tuple[bool, np.ndarray | None]:
    residual = graph.residual_formula()
    assign = np.zeros(graph.n, dtype=np.bool_)
    for i in range(graph.n):
        if graph.fixed[i]:
            assign[i] = graph.value[i]
        else:
            assign[i] = bool(rng.random() < 0.5)
    if residual.m == 0:
        ok = graph.formula_satisfied(assign)
        return ok, assign if ok else None
    success, assign = walksat(
        graph.n,
        residual.clauses,
        residual.signs,
        assign,
        frozen=graph.fixed,
        max_flips=cfg.walksat_flips,
        rng=rng,
    )
    if not success:
        return False, None
    merged = graph.full_assignment_from_residual(assign)
    if not graph.formula_satisfied(merged):
        return False, None
    return True, merged


def run_bsp(
    formula: SATFormula,
    cfg: BSPConfig | None = None,
    graph: FactorGraph | None = None,
    n_dec: float = 1.0,
    n_back: float = 1.0,
    last_dec_batch: int = 0,
    on_step: OnStep | None = None,
    rng: np.random.Generator | None = None,
) -> BSPResult:
    """Run SID/BSP. If `graph` is given, continue from that residual state."""
    if cfg is None:
        cfg = BSPConfig()
    if rng is None:
        rng = np.random.default_rng(cfg.seed)
    if graph is None:
        graph = FactorGraph(formula, rng=rng)
        try:
            graph.unit_propagate()
        except ContradictionError as e:
            return BSPResult(False, f"contradiction:{e}", [], None, graph)

    traj: list[StepRecord] = []
    step = 0
    max_steps = cfg.max_steps
    if max_steps is None:
        denom = max(1.0 - cfg.r_bsp, 0.05)
        max_steps = int(8.0 / max(cfg.frac, 1e-6) / denom) + formula.n + 50
    try:
        while True:
            if step >= max_steps:
                return BSPResult(False, "max_steps", traj, None, graph)
            iterate_sp(graph, cfg.sp)
            if n_back / n_dec < cfg.r_bsp:
                n_back += 1.0
                next_back = True
            else:
                n_dec += 1.0
                next_back = False

            rec = StepRecord(
                step=step,
                L=graph.n_t,
                M_t=graph.m_t,
                sigma=float(graph.complexity),
                sigma_per_n=float(graph.complexity) / float(graph.n) if graph.n else 0.0,
                F=polarization_f(graph),
                eta=int(graph.eta),
                next_move="back" if next_back else "dec",
                n_fixed=len(graph.fixed_list),
                unit_prop=int(graph.unit_prop_count),
                n_dec=n_dec,
                n_back=n_back,
            )
            traj.append(rec)
            if on_step is not None:
                on_step(graph, rec)

            if graph.complexity == 0.0 or graph.n_t == 0:
                rec.next_move = "para"
                ok, assign = _try_walksat(graph, cfg, rng)
                if ok:
                    return BSPResult(True, "paramagnetic", traj, assign, graph)
                return BSPResult(False, "walksat_fail", traj, None, graph)
            if graph.complexity < cfg.negative_sigma:
                rec.next_move = "fail"
                return BSPResult(False, "negative_sigma", traj, None, graph)

            if next_back:
                backtrack(graph, last_dec_batch)
            else:
                if graph.n_t == 0:
                    continue
                last_dec_batch = decimate(graph, cfg, rng)
            step += 1
    except ContradictionError as e:
        return BSPResult(False, f"contradiction:{e}", traj, None, graph)
    except SPNoConvergence as e:
        return BSPResult(False, f"no_convergence:{e}", traj, None, graph)
    except NegativeComplexityError as e:
        return BSPResult(False, f"negative_sigma:{e}", traj, None, graph)
    except BSPError as e:
        return BSPResult(False, str(e), traj, None, graph)


def solve_random(n: int, alpha: float, k: int = 3, cfg: BSPConfig | None = None) -> BSPResult:
    if cfg is None:
        cfg = BSPConfig()
    formula = generate_random_ksat(n, alpha, k=k, seed=cfg.seed)
    return run_bsp(formula, cfg=cfg)


def main(argv: list[str] | None = None) -> int:
    import argparse

    p = argparse.ArgumentParser(description="Python SID/BSP (C++-faithful baseline)")
    p.add_argument("-w", nargs=3, metavar=("K", "ALPHA", "N"), help="generate random K-SAT")
    p.add_argument("-l", metavar="FILE", help="load DIMACS CNF")
    p.add_argument("--r", type=float, default=0.9, help="C++ r = n_back/n_dec in [0,1)")
    p.add_argument("--r-parisi", type=float, default=None, help="Parisi r = n_back/total in [0,0.5)")
    p.add_argument("--f", type=float, default=0.00125, help="decimation fraction")
    p.add_argument("--seed", type=int, default=None)
    p.add_argument("--eps", type=float, default=0.01)
    p.add_argument("--quiet", action="store_true")
    args = p.parse_args(argv)

    r = parisi_r_to_cpp(args.r_parisi) if args.r_parisi is not None else args.r
    cfg = BSPConfig(r_bsp=r, frac=args.f, seed=args.seed, sp=SPConfig(epsilon=args.eps))

    if args.l:
        formula = load_dimacs(args.l)
    elif args.w:
        k, alpha, n = int(args.w[0]), float(args.w[1]), int(args.w[2])
        formula = generate_random_ksat(n, alpha, k=k, seed=args.seed)
    else:
        p.error("need -w K ALPHA N or -l FILE")
        return 2

    result = run_bsp(formula, cfg=cfg)
    if not args.quiet:
        print(f"success={result.success} reason={result.reason} steps={len(result.trajectory)}")
        if result.trajectory:
            last = result.trajectory[-1]
            print(
                f"L={last.L} M_t={last.M_t} Sigma={last.sigma:.6g} "
                f"Sigma/N={last.sigma_per_n:.6g} F={last.F:.6g}"
            )
    return 0 if result.success else 1


if __name__ == "__main__":
    raise SystemExit(main())
