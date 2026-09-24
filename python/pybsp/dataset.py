"""Index RandSATBench-style CNF trees (N*_M*_id*.cnf) and ground-truth labels."""

from __future__ import annotations

import re
from pathlib import Path

import pandas as pd

from pybsp.grid import expand_seeds

CNF_NAME = re.compile(r"^N(\d+)_M(\d+)_id(\d+)\.cnf$")
SAT_MAP = {0: "UNSAT", 1: "SAT", "0": "UNSAT", "1": "SAT"}


def parse_cnf_name(path: Path) -> tuple[int, int, int, float] | None:
    m = CNF_NAME.match(path.name)
    if not m:
        return None
    N, M, inst = int(m[1]), int(m[2]), int(m[3])
    return N, M, inst, round(M / N, 6)


def parse_N_spec(spec) -> list[int] | None:
    """None → all N in the dataset. int or list[int] → those sizes only."""
    if spec is None:
        return None
    if isinstance(spec, list):
        return [int(x) for x in spec]
    return [int(spec)]


def index_cnfs(dataset: Path, N: int | list[int] | None = None) -> pd.DataFrame:
    """One row per CNF: path, name, N, M, seed (=id), alpha (=M/N)."""
    if isinstance(N, list):
        frames = [index_cnfs(dataset, n) for n in N]
        return pd.concat(frames, ignore_index=True) if frames else pd.DataFrame()
    pattern = f"N{N}_*.cnf" if N is not None else "*.cnf"
    rows = []
    for path in sorted(dataset.glob(pattern)):
        parsed = parse_cnf_name(path)
        if parsed is None:
            continue
        n, M, seed, alpha = parsed
        rows.append(
            {
                "path": str(path.resolve()),
                "name": path.name,
                "N": n,
                "M": M,
                "seed": seed,
                "alpha": alpha,
            }
        )
    return pd.DataFrame(rows)


def snap_alpha(available: list[float], requested: float, tol: float) -> float:
    """Nearest available M/N within tol."""
    if not available:
        raise ValueError("dataset has no CNFs for the requested N")
    best = float(min(available, key=lambda x: abs(x - requested)))
    if abs(best - requested) > tol:
        raise ValueError(
            f"no CNF alpha within tol={tol} of {requested}; nearest is {best}. "
            f"available={sorted(available)}"
        )
    return best


def select_instances(
    index: pd.DataFrame,
    *,
    N: int | list[int] | None,
    alphas: list[float],
    ids,
    alpha_tol: float = 0.06,
) -> tuple[pd.DataFrame, dict[int, dict[float, float]]]:
    """Filter to N × snapped alphas × ids.

    N=None uses every size present in index. Returns (instances, {N: req→snapped}).
    """
    if index.empty:
        raise ValueError("empty CNF index")
    n_list = parse_N_spec(N)
    if n_list is None:
        n_list = sorted(int(x) for x in index["N"].unique())
    req = [float(a) for a in alphas]
    id_list = expand_seeds(ids)
    parts: list[pd.DataFrame] = []
    alpha_maps: dict[int, dict[float, float]] = {}
    for n in n_list:
        sub = index.loc[index["N"] == n]
        if sub.empty:
            raise ValueError(f"no CNFs with N={n} in dataset")
        available = sorted(float(x) for x in sub["alpha"].unique())
        req_map = {a: snap_alpha(available, a, alpha_tol) for a in req}
        alpha_maps[n] = req_map
        snapped = list(dict.fromkeys(req_map.values()))
        picked = sub.loc[sub["alpha"].isin(snapped) & sub["seed"].isin(id_list)]
        if picked.empty:
            raise ValueError(f"no instances match N={n}, alphas, and ids")
        parts.append(picked)
    out = pd.concat(parts, ignore_index=True).sort_values(["N", "alpha", "seed"])
    return out.reset_index(drop=True), alpha_maps


def labels_for_instances(labels_csv: Path, instances: pd.DataFrame) -> pd.DataFrame:
    """Join RandSATBench labels onto selected instances → N, alpha, seed, sat."""
    lab = pd.read_csv(labels_csv, usecols=["cnf_file", "sat"])
    lab = lab.assign(name=lab["cnf_file"].map(lambda s: Path(s).name))
    lab["sat"] = lab["sat"].map(SAT_MAP)
    if lab["sat"].isna().any():
        bad = lab.loc[lab["sat"].isna(), "sat"].unique()
        raise ValueError(f"unknown sat codes in {labels_csv}: {bad}")
    merged = instances.merge(lab[["name", "sat"]], on="name", how="left")
    missing = int(merged["sat"].isna().sum())
    if missing:
        raise ValueError(f"{missing} selected CNFs missing from {labels_csv}")
    return merged[["N", "alpha", "seed", "sat"]].drop_duplicates()


def write_sat_labels(path: Path, labels: pd.DataFrame) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    labels.to_csv(path, index=False, compression="gzip")


def label_merge_keys(runs: pd.DataFrame, labels: pd.DataFrame) -> list[str]:
    """Join keys shared by runs and labels (prefer N when present)."""
    return [c for c in ("N", "alpha", "seed") if c in runs.columns and c in labels.columns]
