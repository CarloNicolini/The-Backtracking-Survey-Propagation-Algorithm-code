"""Cartesian product of configs × alphas × seeds; write csv.gz."""

from __future__ import annotations

import json
import subprocess
from concurrent.futures import ProcessPoolExecutor, as_completed
from itertools import product
from pathlib import Path

import pandas as pd

from pybsp.parse import load_csv_gz, summarize_run
from pybsp.runner import run_trial

RUNS_COLS = [
    "cfg",
    "K",
    "N",
    "alpha",
    "seed",
    "status",
    "returncode",
    "wall_s",
    "n_steps",
    "sigma_last",
    "sigma_last_over_N",
    "sigma_res",
    "sigma_res_over_N",
]

STEPS_COLS = [
    "cfg",
    "K",
    "N",
    "alpha",
    "seed",
    "step",
    "Nt",
    "Mt",
    "Sigma",
    "Sigma_over_N",
    "alpha_res",
    "y",
    "E_per_Nt",
]


def expand_seeds(spec) -> list[int]:
    if isinstance(spec, dict):
        start = int(spec["start"])
        stop = int(spec["stop"])
        return list(range(start, stop + 1))
    return [int(s) for s in spec]


def load_grid(path: Path) -> dict:
    with path.open() as f:
        return json.load(f)


def trial_tag(cfg: str, alpha: float, seed: int, N: int | None = None) -> str:
    if N is None:
        return f"{cfg}_a{alpha}_s{seed}"
    return f"{cfg}_N{N}_a{alpha}_s{seed}"


def done_keys(runs: pd.DataFrame) -> set[tuple]:
    if runs.empty:
        return set()
    if "N" in runs.columns:
        return {
            (str(c), int(n), float(a), int(s))
            for c, n, a, s in zip(runs["cfg"], runs["N"], runs["alpha"], runs["seed"])
        }
    return {
        (str(c), float(a), int(s))
        for c, a, s in zip(runs["cfg"], runs["alpha"], runs["seed"])
    }


def _one_trial(payload: dict) -> tuple[dict, list[dict]]:
    rundir = Path(payload["rundir"])
    cnf = Path(payload["cnf"]) if payload.get("cnf") else None
    text, rc, wall_s, timed_out = run_trial(
        Path(payload["bsp"]),
        K=payload["K"],
        alpha=payload["alpha"],
        N=payload["N"],
        seed=payload["seed"],
        flags=payload["flags"],
        rundir=rundir,
        timeout_s=payload["timeout_s"],
        cnf=cnf,
    )
    summary, steps = summarize_run(
        text,
        timed_out=timed_out,
        returncode=rc,
        wall_s=wall_s,
        diag_steps=rundir / "t_steps.csv",
    )
    run_row = {
        "cfg": payload["cfg"],
        "K": payload["K"],
        "N": payload["N"],
        "alpha": payload["alpha"],
        "seed": payload["seed"],
        **summary,
    }
    step_rows = [
        {
            "cfg": payload["cfg"],
            "K": payload["K"],
            "N": payload["N"],
            "alpha": payload["alpha"],
            "seed": payload["seed"],
            "step": s["step"],
            "Nt": s["Nt"],
            "Mt": s["Mt"],
            "Sigma": s["Sigma"],
            "Sigma_over_N": s["Sigma_over_N"],
            "alpha_res": s.get("alpha_res", float("nan")),
            "y": s.get("y", float("nan")),
            "E_per_Nt": s.get("E_per_Nt", float("nan")),
        }
        for s in steps
    ]
    return run_row, step_rows


def append_csv_gz(path: Path, df: pd.DataFrame, columns: list[str]) -> None:
    if df.empty:
        return
    df = df.reindex(columns=columns)
    if path.is_file():
        df.to_csv(path, mode="a", header=False, index=False, compression="gzip")
    else:
        df.to_csv(path, index=False, compression="gzip")


def git_state(repo: Path) -> dict:
    """Commit and dirty flag of the repository that holds the bsp binary."""
    git = ["git", "-C", str(repo)]
    commit = subprocess.run([*git, "rev-parse", "HEAD"], capture_output=True, text=True)
    status = subprocess.run([*git, "status", "--porcelain", "--untracked-files=no"],
                            capture_output=True, text=True)
    return {"commit": commit.stdout.strip() or None, "dirty": bool(status.stdout.strip())}


def run_grid(grid_path: Path, outdir: Path, jobs: int | None = None) -> None:
    spec = load_grid(grid_path)
    if "dataset" in spec:
        from pybsp.load_grid import run_load_grid

        run_load_grid(grid_path, outdir, jobs=jobs)
        return
    outdir.mkdir(parents=True, exist_ok=True)
    runs_path = outdir / "runs.csv.gz"
    steps_path = outdir / "steps.csv.gz"
    runs_dir = outdir / "runs"
    runs_dir.mkdir(exist_ok=True)

    bsp = Path(spec["bsp"])
    if not bsp.is_file():
        cand = Path.cwd() / bsp
        if cand.is_file():
            bsp = cand
        else:
            raise FileNotFoundError(f"bsp binary not found: {spec['bsp']}")

    # Experiment-level manifest: full grid JSON plus resolved paths.
    from datetime import datetime, timezone

    exp_manifest = {
        "kind": "experiment",
        "grid_path": str(grid_path.resolve()),
        "outdir": str(outdir.resolve()),
        "bsp": str(bsp.resolve()),
        "git": git_state(bsp.resolve().parent),
        "timestamp_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "grid": spec,
    }
    (outdir / "manifest.json").write_text(json.dumps(exp_manifest, indent=2) + "\n")

    K = int(spec["K"])
    N = int(spec["N"])
    alphas = [float(a) for a in spec["alphas"]]
    seeds = expand_seeds(spec["seeds"])
    timeout_s = float(spec.get("timeout_s", 300))
    n_jobs = int(jobs if jobs is not None else spec.get("jobs", 1))
    extra = list(spec.get("extra_flags", []))
    configs = spec["configs"]

    existing = load_csv_gz(runs_path)
    skip = done_keys(existing)

    payloads = []
    for cfg, alpha, seed in product(configs, alphas, seeds):
        name = cfg["name"]
        key = (name, N, float(alpha), int(seed))
        if key in skip:
            continue
        flags = list(extra) + list(cfg.get("flags", []))
        payloads.append(
            {
                "bsp": str(bsp.resolve()),
                "cfg": name,
                "K": K,
                "N": N,
                "alpha": alpha,
                "seed": seed,
                "flags": flags,
                "timeout_s": timeout_s,
                "rundir": str(runs_dir / trial_tag(name, alpha, seed, N=N)),
            }
        )

    if not payloads:
        print(f"nothing to run; {len(skip)} trials already in {runs_path}")
        return

    print(f"running {len(payloads)} trials (skip {len(skip)}), jobs={n_jobs}")

    if n_jobs <= 1:
        for p in payloads:
            r, s = _one_trial(p)
            print(f"  {r['cfg']} a={r['alpha']} s={r['seed']} -> {r['status']}")
            append_csv_gz(runs_path, pd.DataFrame([r]), RUNS_COLS)
            if s:
                append_csv_gz(steps_path, pd.DataFrame(s), STEPS_COLS)
    else:
        with ProcessPoolExecutor(max_workers=n_jobs) as pool:
            futs = {pool.submit(_one_trial, p): p for p in payloads}
            for fut in as_completed(futs):
                r, s = fut.result()
                print(f"  {r['cfg']} a={r['alpha']} s={r['seed']} -> {r['status']}")
                append_csv_gz(runs_path, pd.DataFrame([r]), RUNS_COLS)
                if s:
                    append_csv_gz(steps_path, pd.DataFrame(s), STEPS_COLS)

    print(f"wrote {runs_path}")
    if steps_path.is_file():
        print(f"wrote {steps_path}")
    print(f"wrote {outdir / 'manifest.json'}")
