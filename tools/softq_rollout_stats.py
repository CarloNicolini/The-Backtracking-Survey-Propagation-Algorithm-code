"""Predictivity test of the soft look-ahead score from rollout runs.

Each trial directory holds softq_rollouts.csv (one row per candidate at each
checkpoint) and rollout_f<f>_c<j>/ with the plain-BSP continuation of that
candidate. The test asks whether the score ranks the rollouts better than
the BSP order, within groups of candidates of the same checkpoint.

Usage: python tools/softq_rollout_stats.py <grid outdir>
"""

from __future__ import annotations

import sys
from itertools import combinations
from pathlib import Path

import pandas as pd

from pybsp.parse import classify_status, read_diag_steps

GRID = ["q1a0", "q2a0", "q4a0", "q1a005", "q2a005"]  # g_softq_score_grid in softq.cpp
COLS = ["f", "nfixed", "cand", "vertex", "bias", "sigma_probe", "sigma", "score", *GRID]


def rollout_row(trial: Path, f: float, cand: int) -> dict:
    d = trial / f"rollout_f{f:g}_c{cand}"
    steps = read_diag_steps(d / "t_steps.csv")
    drops = [max(0.0, a["Sigma"] - b["Sigma"]) for a, b in zip(steps, steps[1:])]
    return {
        "solved": classify_status((d / "log.txt").read_text()) == "solve",
        "n_steps": len(steps),
        "max_drop": max(drops, default=0.0),
        "final_nt": steps[-1]["Nt"] if steps else float("nan"),
    }


def load(outdir: Path) -> pd.DataFrame:
    rows = []
    for csv in sorted(outdir.glob("runs/*/softq_rollouts.csv")):
        trial = csv.parent
        df = pd.read_csv(csv, names=COLS)
        for r in df.itertuples():
            rows.append({"trial": trial.name, **r._asdict(), **rollout_row(trial, r.f, r.cand)})
    return pd.DataFrame(rows)


def pair_stats(df: pd.DataFrame, key: str, target: str) -> tuple[int, int]:
    """Concordant and discordant pairs of (key, target) within each group."""
    conc = disc = 0
    for _, g in df.groupby(["trial", "f"]):
        for a, b in combinations(g.itertuples(), 2):
            dt = getattr(a, target) - getattr(b, target)
            dk = getattr(a, key) - getattr(b, key)
            if dt == 0 or dk == 0:
                continue
            conc += dt * dk > 0
            disc += dt * dk < 0
    return conc, disc


def main(outdir: Path) -> None:
    df = load(outdir)
    df["neg_rank"] = -df["cand"]
    df["neg_max_drop"] = -df["max_drop"]
    groups = df.groupby(["trial", "f"])
    mixed = groups["solved"].transform(lambda s: 0 < s.sum() < len(s))
    print(f"{df.trial.nunique()} trials, {groups.ngroups} checkpoints, "
          f"{int(mixed.sum())} rollouts in groups with mixed outcome")
    print(df.groupby("f")["solved"].mean().rename("P(solve) of rollouts").to_string())
    for target in ["solved", "neg_max_drop"]:
        for key in ["neg_rank", "bias", *GRID]:
            c, d = pair_stats(df, key, target)
            tau = (c - d) / (c + d) if c + d else float("nan")
            print(f"{target:>12} vs {key:<8}: concordant={c} discordant={d} tau={tau:+.3f}")
    bsp = df[df["cand"] == 0]
    print(f"P(solve) BSP first candidate = {bsp.solved.mean():.3f}, n={len(bsp)}")
    for key in GRID:
        top = df.loc[groups[key].idxmax()]
        print(f"P(solve) best {key} candidate = {top.solved.mean():.3f}")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
