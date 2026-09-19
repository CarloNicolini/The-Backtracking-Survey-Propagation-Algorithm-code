#!/usr/bin/env python3
"""Reproduce Parisi cond-mat/0308510 Figs. 1–2: Σ(L) and F(L).

Uses the C++/Nature backtracking ratio r_cpp = r_p / (1-r_p) so that
Parisi r ∈ {0, 0.25, 0.4} is not confused with the C++ default r=0.9.
"""

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))

from baseline_bsp import BSPConfig, parisi_r_to_cpp, run_bsp  # noqa: E402
from sat_instance import generate_random_ksat  # noqa: E402
from survey_propagation import SPConfig  # noqa: E402

PARISI_RS = (0.0, 0.25, 0.4)


def run_one(n: int, alpha: float, r_p: float, f: float, seed: int) -> list[dict]:
    formula = generate_random_ksat(n, alpha, k=3, seed=seed)
    cfg = BSPConfig(
        r_bsp=parisi_r_to_cpp(r_p),
        frac=f,
        seed=seed,
        sp=SPConfig(),
    )
    result = run_bsp(formula, cfg=cfg)
    rows = []
    for rec in result.trajectory:
        rows.append(
            {
                "r_parisi": r_p,
                "r_cpp": cfg.r_bsp,
                "seed": seed,
                "success": int(result.success),
                "reason": result.reason,
                "step": rec.step,
                "L": rec.L,
                "M_t": rec.M_t,
                "sigma": rec.sigma,
                "sigma_per_n": rec.sigma_per_n,
                "F": rec.F,
                "eta": rec.eta,
                "next_move": rec.next_move,
            }
        )
    return rows


def write_csv(path: Path, rows: list[dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        path.write_text("")
        return
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)


def plot_fig1_fig2(rows: list[dict], out_png: Path, n: int, alpha: float) -> None:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    fig, axes = plt.subplots(1, 2, figsize=(10, 4))
    styles = {0.0: ("C0", "-"), 0.25: ("C1", "--"), 0.4: ("C2", "-.")}
    for r_p in PARISI_RS:
        sub = [row for row in rows if row["r_parisi"] == r_p]
        if not sub:
            continue
        L = [row["L"] for row in sub]
        sig = [row["sigma"] for row in sub]
        F = [row["F"] for row in sub]
        color, ls = styles[r_p]
        axes[0].plot(L, sig, color=color, ls=ls, label=rf"$r_p={r_p}$")
        axes[1].plot(L, F, color=color, ls=ls, label=rf"$r_p={r_p}$")
    axes[0].set_xlabel("L (unassigned)")
    axes[0].set_ylabel(r"$\Sigma$")
    axes[0].set_title(rf"Fig. 1  $N={n}$, $\alpha={alpha}$")
    axes[0].legend()
    axes[1].set_xlabel("L (unassigned)")
    axes[1].set_ylabel("F(L)")
    axes[1].set_title(rf"Fig. 2  $N={n}$, $\alpha={alpha}$")
    axes[1].legend()
    fig.tight_layout()
    out_png.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_png, dpi=140)
    plt.close(fig)


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--n", type=int, default=1000, help="N=1e4 regression, N=1e5 full-scale")
    p.add_argument("--alpha", type=float, default=4.25)
    p.add_argument("--f", type=float, default=1e-3)
    p.add_argument("--seed", type=int, default=1)
    p.add_argument("--outdir", type=Path, default=ROOT / "output" / "fig1_fig2")
    args = p.parse_args(argv)

    all_rows: list[dict] = []
    for r_p in PARISI_RS:
        print(f"running N={args.n} alpha={args.alpha} r_parisi={r_p} -> r_cpp={parisi_r_to_cpp(r_p):.4f}")
        rows = run_one(args.n, args.alpha, r_p, args.f, args.seed)
        all_rows.extend(rows)
        if rows:
            last = rows[-1]
            print(
                f"  done reason={last['reason']} success={last['success']} "
                f"steps={len(rows)} L_final={last['L']} Sigma={last['sigma']:.4g} F={last['F']:.4g}"
            )

    csv_path = args.outdir / f"sigma_F_N{args.n}_a{args.alpha}_s{args.seed}.csv"
    png_path = args.outdir / f"fig1_fig2_N{args.n}_a{args.alpha}_s{args.seed}.png"
    write_csv(csv_path, all_rows)
    plot_fig1_fig2(all_rows, png_path, args.n, args.alpha)
    print(f"wrote {csv_path}")
    print(f"wrote {png_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
