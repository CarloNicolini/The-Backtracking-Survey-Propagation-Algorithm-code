#!/usr/bin/env python3
"""Markovianity diagnostic for the proposed RL observation.

The true state is the residual factor graph plus cavity messages. The proposed
compression φ is (local-survey summary, L, Σ). If two histories land at similar
φ but then disagree on future I(k) and future Σ under the same SID continuation,
φ is not a sufficient statistic and must be enriched (radius-2–3 subgraph stats)
before BSPDecimationEnv.

This script is a gate: do not build the Gym env until the report is reviewed.
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))

from baseline_bsp import BSPConfig, run_bsp  # noqa: E402
from sat_instance import FactorGraph, generate_random_ksat  # noqa: E402
from survey_propagation import SPConfig, state_features  # noqa: E402


@dataclass
class Snapshot:
    instance_id: int
    history_id: int
    step: int
    phi: np.ndarray
    L: int
    sigma: float
    frozen: frozenset
    i_stale: np.ndarray  # certitude on all vars; frozen slots are I(k)
    graph: FactorGraph


def collect_history(
    instance_id: int,
    history_id: int,
    formula,
    cfg: BSPConfig,
) -> list[Snapshot]:
    snaps: list[Snapshot] = []

    def on_step(graph: FactorGraph, rec) -> None:
        snaps.append(
            Snapshot(
                instance_id=instance_id,
                history_id=history_id,
                step=rec.step,
                phi=state_features(graph),
                L=rec.L,
                sigma=rec.sigma,
                frozen=frozenset(int(i) for i in graph.fixed_list),
                i_stale=graph.sC.copy(),
                graph=graph.copy(),
            )
        )

    run_bsp(formula, cfg=cfg, on_step=on_step)
    return snaps


def jaccard_disagreement(a: frozenset, b: frozenset) -> float:
    if not a and not b:
        return 0.0
    inter = len(a & b)
    union = len(a | b)
    return 1.0 - inter / union if union else 0.0


def match_pairs(
    snaps: list[Snapshot],
    phi_tol: float,
    min_jaccard: float,
    l_bin: int,
) -> list[tuple[Snapshot, Snapshot, float]]:
    """Pairs from different histories, similar φ, different frozen sets."""
    pairs = []
    for i, a in enumerate(snaps):
        for b in snaps[i + 1 :]:
            if a.instance_id != b.instance_id:
                continue
            if a.history_id == b.history_id:
                continue
            if abs(a.L - b.L) > l_bin:
                continue
            dist = float(np.linalg.norm(a.phi - b.phi))
            if dist > phi_tol:
                continue
            if jaccard_disagreement(a.frozen, b.frozen) < min_jaccard:
                continue
            pairs.append((a, b, dist))
    return pairs


def same_history_noise(snaps: list[Snapshot], lag: int = 1) -> list[tuple[float, float]]:
    """(|ΔΣ|, I-vector L2) between consecutive steps of the same history."""
    out = []
    by_hist: dict[tuple[int, int], list[Snapshot]] = {}
    for s in snaps:
        by_hist.setdefault((s.instance_id, s.history_id), []).append(s)
    for seq in by_hist.values():
        seq = sorted(seq, key=lambda x: x.step)
        for i in range(len(seq) - lag):
            a, b = seq[i], seq[i + lag]
            d_sig = abs(a.sigma - b.sigma)
            d_i = float(np.linalg.norm(a.i_stale - b.i_stale))
            out.append((d_sig, d_i))
    return out


def continue_sid(snap: Snapshot, horizon: int, seed: int) -> tuple[list[float], np.ndarray, str]:
    cfg = BSPConfig(
        r_bsp=0.0,
        frac=0.05,
        seed=seed,
        max_steps=horizon,
        sp=SPConfig(t_max=64),
    )
    result = run_bsp(
        snap.graph.formula,
        cfg=cfg,
        graph=snap.graph.copy(),
        n_dec=1.0,
        n_back=1.0,
    )
    sigmas = [rec.sigma for rec in result.trajectory]
    i_final = result.graph.sC.copy()
    return sigmas, i_final, result.reason


def ik_rank_correlation(a: Snapshot, b: Snapshot) -> float:
    """Spearman-like correlation of stale I(k) on the intersection of frozen vars."""
    common = sorted(a.frozen & b.frozen)
    if len(common) < 3:
        return float("nan")
    xa = np.array([a.i_stale[i] for i in common])
    xb = np.array([b.i_stale[i] for i in common])
    ra = np.argsort(np.argsort(xa))
    rb = np.argsort(np.argsort(xb))
    if ra.std() == 0 or rb.std() == 0:
        return float("nan")
    return float(np.corrcoef(ra, rb)[0, 1])


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--n", type=int, default=80)
    p.add_argument("--alpha", type=float, default=4.20)
    p.add_argument("--instances", type=int, default=3)
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--horizon", type=int, default=8)
    p.add_argument("--phi-tol", type=float, default=1.25)
    p.add_argument("--min-jaccard", type=float, default=0.08)
    p.add_argument("--outdir", type=Path, default=ROOT / "output" / "markovianity")
    args = p.parse_args(argv)
    args.outdir.mkdir(parents=True, exist_ok=True)

    rng = np.random.default_rng(args.seed)
    all_snaps: list[Snapshot] = []
    history_cfgs = [
        BSPConfig(r_bsp=0.0, frac=0.1, max_steps=12, seed=args.seed, sp=SPConfig(t_max=64)),
        BSPConfig(r_bsp=0.5, frac=0.1, max_steps=12, seed=args.seed + 1, sp=SPConfig(t_max=64)),
        BSPConfig(
            r_bsp=0.0,
            frac=0.1,
            max_steps=12,
            seed=args.seed + 2,
            noisy_topk=5,
            sp=SPConfig(t_max=64),
        ),
    ]

    for inst in range(args.instances):
        seed = int(args.seed + 1000 * inst)
        formula = generate_random_ksat(args.n, args.alpha, k=3, seed=seed)
        for h, cfg in enumerate(history_cfgs):
            cfg = BSPConfig(
                r_bsp=cfg.r_bsp,
                frac=cfg.frac,
                max_steps=cfg.max_steps,
                seed=cfg.seed,
                noisy_topk=cfg.noisy_topk,
                sp=cfg.sp,
            )
            snaps = collect_history(inst, h, formula, cfg)
            all_snaps.extend(snaps)
            print(f"instance {inst} history {h}: {len(snaps)} snapshots")

    pairs = match_pairs(all_snaps, phi_tol=args.phi_tol, min_jaccard=args.min_jaccard, l_bin=max(2, args.n // 20))
    noise = same_history_noise(all_snaps, lag=1)
    noise_sig = float(np.mean([x[0] for x in noise])) if noise else float("nan")
    noise_i = float(np.mean([x[1] for x in noise])) if noise else float("nan")

    rows = []
    future_sig_gaps = []
    future_i_gaps = []
    rank_corrs = []
    for a, b, dist in pairs:
        sig_a, i_a, ra = continue_sid(a, args.horizon, seed=int(rng.integers(1, 10**6)))
        sig_b, i_b, rb = continue_sid(b, args.horizon, seed=int(rng.integers(1, 10**6)))
        h = min(len(sig_a), len(sig_b), args.horizon)
        if h == 0:
            continue
        gap_sig = float(np.sqrt(np.mean((np.array(sig_a[:h]) - np.array(sig_b[:h])) ** 2)))
        gap_i = float(np.linalg.norm(i_a - i_b) / max(np.sqrt(a.graph.n), 1.0))
        rho = ik_rank_correlation(a, b)
        future_sig_gaps.append(gap_sig)
        future_i_gaps.append(gap_i)
        if np.isfinite(rho):
            rank_corrs.append(rho)
        rows.append(
            {
                "instance": a.instance_id,
                "hist_a": a.history_id,
                "hist_b": b.history_id,
                "L_a": a.L,
                "L_b": b.L,
                "phi_dist": dist,
                "jaccard_dis": jaccard_disagreement(a.frozen, b.frozen),
                "sigma_a": a.sigma,
                "sigma_b": b.sigma,
                "future_sigma_rmse": gap_sig,
                "future_I_l2": gap_i,
                "I_rank_corr": rho,
                "reason_a": ra,
                "reason_b": rb,
                "same_terminal": int(ra == rb),
            }
        )

    mean_gap = float(np.mean(future_sig_gaps)) if future_sig_gaps else float("nan")
    mean_i = float(np.mean(future_i_gaps)) if future_i_gaps else float("nan")
    mean_rho = float(np.mean(rank_corrs)) if rank_corrs else float("nan")
    # Gate: large if future Σ RMSE is several times the same-history noise floor
    # and I(k) rank correlation is weak.
    ratio = mean_gap / noise_sig if (noise_sig and noise_sig > 1e-12 and np.isfinite(mean_gap)) else float("nan")
    failed = bool(np.isfinite(ratio) and ratio > 3.0 and (not np.isfinite(mean_rho) or mean_rho < 0.3))
    report = {
        "n": args.n,
        "alpha": args.alpha,
        "n_snapshots": len(all_snaps),
        "n_pairs": len(rows),
        "noise_floor_delta_sigma": noise_sig,
        "noise_floor_delta_I": noise_i,
        "mean_future_sigma_rmse": mean_gap,
        "mean_future_I_l2": mean_i,
        "mean_I_rank_corr": mean_rho,
        "sigma_rmse_over_noise": ratio,
        "gate_failed": failed,
        "verdict": (
            "FAIL: φ aliases future I(k)/Σ; enrich with radius-2–3 residual-subgraph stats before env.py"
            if failed
            else (
                "INCONCLUSIVE: too few matched pairs; re-run with more instances or a looser --phi-tol"
                if len(rows) < 5
                else "PASS: matched-pair future divergence is comparable to the same-history noise floor"
            )
        ),
    }
    csv_path = args.outdir / "pairs.csv"
    json_path = args.outdir / "report.json"
    if rows:
        with csv_path.open("w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
            w.writeheader()
            w.writerows(rows)
    json_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))
    print(f"wrote {json_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
