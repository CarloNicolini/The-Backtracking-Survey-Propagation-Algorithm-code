"""Success rates and paired comparisons of grid configurations."""

from __future__ import annotations

from math import comb

import pandas as pd


def binomial_se(p: float, n: int) -> float:
    if n <= 0:
        return float("nan")
    return (p * (1.0 - p) / n) ** 0.5


def _group_keys(df: pd.DataFrame, extra: list[str] | None = None) -> list[str]:
    keys = ["cfg"]
    if "N" in df.columns:
        keys.append("N")
    keys.append("alpha")
    if extra:
        keys.extend(extra)
    return keys


def success_table(runs: pd.DataFrame) -> pd.DataFrame:
    keys = _group_keys(runs)
    g = runs.groupby(keys, as_index=False).agg(
        n=("status", "size"),
        n_solve=("status", lambda s: int((s == "solve").sum())),
    )
    g["p_solve"] = g["n_solve"] / g["n"]
    g["se"] = [binomial_se(p, n) for p, n in zip(g["p_solve"], g["n"])]
    return g


def mcnemar_p(b: int, c: int) -> float:
    """Exact two-sided McNemar p-value for b and c discordant pairs."""
    n = b + c
    if n == 0:
        return 1.0
    tail = sum(comb(n, k) for k in range(min(b, c) + 1)) / 2**n
    return min(1.0, 2.0 * tail)


def label_agreement_table(runs: pd.DataFrame, labels: pd.DataFrame) -> pd.DataFrame:
    """Per cfg (and N) and alpha: solve rates on SAT / UNSAT ground truth."""
    on = [c for c in ("N", "alpha", "seed") if c in runs.columns and c in labels.columns]
    m = runs.merge(labels, on=on, how="inner")
    keys = _group_keys(m, ["sat"])
    rows = []
    for key, g in m.groupby(keys):
        vals = key if isinstance(key, tuple) else (key,)
        row = dict(zip(keys, vals))
        n = len(g)
        n_solve = int((g["status"] == "solve").sum())
        p = n_solve / n if n else float("nan")
        row.update(
            {"n": n, "n_solve": n_solve, "p_solve": p, "se": binomial_se(p, n)}
        )
        rows.append(row)
    return pd.DataFrame(rows)


def paired_vs_baseline(runs: pd.DataFrame, baseline: str = "cert") -> pd.DataFrame:
    """Per config and alpha (and N), compare solved seeds with the baseline.

    b counts seeds solved by the baseline only, c seeds solved by the config only.
    """
    pair_on = ["alpha", "seed"] + (["N"] if "N" in runs.columns else [])
    cols = ["cfg", *pair_on, "ok"]
    solved = runs.assign(ok=runs["status"] == "solve")[cols]
    base = solved[solved["cfg"] == baseline].drop(columns="cfg")
    pairs = solved[solved["cfg"] != baseline].merge(
        base, on=pair_on, suffixes=("", "_base")
    )
    group = ["cfg", "alpha"] + (["N"] if "N" in pairs.columns else [])
    rows = []
    for key, g in pairs.groupby(group):
        vals = key if isinstance(key, tuple) else (key,)
        row = dict(zip(group, vals))
        b = int((g["ok_base"] & ~g["ok"]).sum())
        c = int((g["ok"] & ~g["ok_base"]).sum())
        row.update(
            {
                "n": len(g),
                "diff": (c - b) / len(g),
                "b": b,
                "c": c,
                "p_mcnemar": mcnemar_p(b, c),
            }
        )
        rows.append(row)
    return pd.DataFrame(rows)
