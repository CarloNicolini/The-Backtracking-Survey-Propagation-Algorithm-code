"""Regex parsing of bsp stdout/stderr and optional diag CSV."""

from __future__ import annotations

import csv
import re
from pathlib import Path

import pandas as pd

# Graph operator<< : Nt Mt Sigma Sigma/N alpha_res
SIGMA_LINE = re.compile(
    r"^\s*(\d+)\s+(\d+)\s+"
    r"([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)\s+"
    r"([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)\s+"
    r"([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)\s*$"
)

_STATUS_PATTERNS: tuple[tuple[str, re.Pattern[str]], ...] = (
    ("solve", re.compile(r"ASSIGNMENT FOUND")),
    ("fail_walksat", re.compile(r"ASSIGNMENT NOT FOUND")),
    ("fail_contra", re.compile(r"Contradiction found")),
    ("fail_negsig", re.compile(r"Negative complexity")),
    ("fail_noconv", re.compile(r"does not converge")),
)


def classify_status(text: str, *, timed_out: bool = False) -> str:
    if timed_out:
        return "fail_timeout"
    for name, pat in _STATUS_PATTERNS:
        if pat.search(text):
            return name
    return "fail_noconv"


def parse_sigma_lines(text: str) -> list[dict]:
    rows: list[dict] = []
    for line in text.splitlines():
        m = SIGMA_LINE.match(line)
        if not m:
            continue
        rows.append(
            {
                "Nt": int(m.group(1)),
                "Mt": int(m.group(2)),
                "Sigma": float(m.group(3)),
                "Sigma_over_N": float(m.group(4)),
                "alpha_res": float(m.group(5)),
            }
        )
    return [{**row, "step": i} for i, row in enumerate(rows)]


def read_diag_steps(path: Path) -> list[dict]:
    if not path.is_file():
        return []
    rows: list[dict] = []
    with path.open() as f:
        lines = [ln for ln in f if not ln.startswith("#")]
    if not lines:
        return []
    reader = csv.DictReader(lines)
    for i, r in enumerate(reader):
        try:
            rows.append(
                {
                    "step": int(r.get("step", i)),
                    "Nt": int(float(r["Nt"])),
                    "Mt": int(float(r["Mt"])),
                    "Sigma": float(r["Sigma"]),
                    "Sigma_over_N": float(r["Sigma_per_N"]),
                    "alpha_res": float("nan"),
                    "eta": float(r["eta"]) if "eta" in r else float("nan"),
                    "move": r.get("move", ""),
                }
            )
        except (KeyError, ValueError):
            continue
    return rows


def sigma_residual(steps: list[dict]) -> tuple[float, float]:
    """Last strictly positive Sigma and Sigma_over_N; else (0, 0)."""
    for row in reversed(steps):
        if row["Sigma"] > 1e-9:
            return row["Sigma"], row["Sigma_over_N"]
    return 0.0, 0.0


def summarize_run(
    text: str,
    *,
    timed_out: bool = False,
    returncode: int = 0,
    wall_s: float = 0.0,
    diag_steps: Path | None = None,
) -> tuple[dict, list[dict]]:
    status = classify_status(text, timed_out=timed_out)
    steps = parse_sigma_lines(text)
    if diag_steps is not None:
        diag = read_diag_steps(diag_steps)
        if diag:
            steps = diag
    n_steps = len(steps)
    if n_steps:
        last = steps[-1]
        sigma_last = last["Sigma"]
        sigma_last_over_N = last["Sigma_over_N"]
    else:
        sigma_last = float("nan")
        sigma_last_over_N = float("nan")
    sigma_res, sigma_res_over_N = sigma_residual(steps)
    if status != "solve":
        sigma_res, sigma_res_over_N = 0.0, 0.0
    summary = {
        "status": status,
        "returncode": returncode,
        "wall_s": wall_s,
        "n_steps": n_steps,
        "sigma_last": sigma_last,
        "sigma_last_over_N": sigma_last_over_N,
        "sigma_res": sigma_res,
        "sigma_res_over_N": sigma_res_over_N,
    }
    return summary, steps


def load_csv_gz(path: Path) -> pd.DataFrame:
    if not path.is_file():
        return pd.DataFrame()
    return pd.read_csv(path, compression="gzip")
