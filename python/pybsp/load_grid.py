"""CNF-load grids over an external dataset (RandSATBench-style)."""

from __future__ import annotations

import json
from concurrent.futures import ProcessPoolExecutor, as_completed
from datetime import datetime, timezone
from pathlib import Path

import pandas as pd

from pybsp.dataset import (
    index_cnfs,
    labels_for_instances,
    parse_N_spec,
    select_instances,
    write_sat_labels,
)
from pybsp.grid import (
    RUNS_COLS,
    STEPS_COLS,
    _one_trial,
    append_csv_gz,
    done_keys,
    git_state,
    load_grid,
    trial_tag,
)
from pybsp.parse import load_csv_gz


def resolve_bsp(spec: dict, cwd: Path) -> Path:
    bsp = Path(spec["bsp"])
    if bsp.is_file():
        return bsp.resolve()
    cand = cwd / bsp
    if cand.is_file():
        return cand.resolve()
    raise FileNotFoundError(f"bsp binary not found: {spec['bsp']}")


def run_load_grid(grid_path: Path, outdir: Path, jobs: int | None = None) -> None:
    """Like run_grid, but each trial loads a CNF (-l) instead of sampling (-w)."""
    spec = load_grid(grid_path)
    outdir.mkdir(parents=True, exist_ok=True)
    runs_path = outdir / "runs.csv.gz"
    steps_path = outdir / "steps.csv.gz"
    runs_dir = outdir / "runs"
    runs_dir.mkdir(exist_ok=True)

    bsp = resolve_bsp(spec, Path.cwd())
    dataset = Path(spec["dataset"]).expanduser()
    if not dataset.is_dir():
        raise FileNotFoundError(f"dataset directory not found: {dataset}")
    labels_csv = Path(spec["labels"]).expanduser()
    if not labels_csv.is_file():
        raise FileNotFoundError(f"labels csv not found: {labels_csv}")

    K = int(spec["K"])
    N_spec = parse_N_spec(spec.get("N"))
    alphas = [float(a) for a in spec["alphas"]]
    alpha_tol = float(spec.get("alpha_tol", 0.06))
    timeout_s = float(spec.get("timeout_s", 300))
    n_jobs = int(jobs if jobs is not None else spec.get("jobs", 1))
    extra = list(spec.get("extra_flags", []))
    configs = spec["configs"]
    ids = spec.get("ids", spec.get("seeds"))
    if ids is None:
        raise KeyError("load grid needs 'ids' (or 'seeds') for CNF instance ids")

    index = index_cnfs(dataset, N=N_spec)
    instances, alpha_maps = select_instances(
        index, N=N_spec, alphas=alphas, ids=ids, alpha_tol=alpha_tol
    )
    labels = labels_for_instances(labels_csv, instances)
    write_sat_labels(outdir / "sat_labels.csv.gz", labels)

    n_values = sorted(int(x) for x in instances["N"].unique())
    exp_manifest = {
        "kind": "experiment",
        "mode": "load",
        "grid_path": str(grid_path.resolve()),
        "outdir": str(outdir.resolve()),
        "bsp": str(bsp),
        "dataset": str(dataset.resolve()),
        "labels": str(labels_csv.resolve()),
        "N": n_values,
        "alpha_map": {
            str(n): {str(k): v for k, v in m.items()} for n, m in alpha_maps.items()
        },
        "n_instances": int(len(instances)),
        "git": git_state(bsp.parent),
        "timestamp_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "grid": spec,
    }
    (outdir / "manifest.json").write_text(json.dumps(exp_manifest, indent=2) + "\n")

    existing = load_csv_gz(runs_path)
    skip = done_keys(existing)

    payloads = []
    for cfg in configs:
        for _, inst in instances.iterrows():
            name = cfg["name"]
            n = int(inst["N"])
            alpha = float(inst["alpha"])
            seed = int(inst["seed"])
            key = (name, n, alpha, seed)
            if key in skip:
                continue
            flags = list(extra) + list(cfg.get("flags", []))
            payloads.append(
                {
                    "bsp": str(bsp),
                    "cfg": name,
                    "K": K,
                    "N": n,
                    "alpha": alpha,
                    "seed": seed,
                    "flags": flags,
                    "timeout_s": timeout_s,
                    "rundir": str(runs_dir / trial_tag(name, alpha, seed, N=n)),
                    "cnf": inst["path"],
                }
            )

    if not payloads:
        print(f"nothing to run; {len(skip)} trials already in {runs_path}")
        return

    print(
        f"running {len(payloads)} load trials "
        f"({len(instances)} CNFs × {len(configs)} cfgs, N={n_values}, "
        f"skip {len(skip)}), jobs={n_jobs}"
    )
    for n, amap in alpha_maps.items():
        print(f"  alpha snap N={n}: {amap}")

    if n_jobs <= 1:
        for p in payloads:
            r, s = _one_trial(p)
            print(
                f"  {r['cfg']} N={r['N']} a={r['alpha']:.4f} id={r['seed']} -> {r['status']}"
            )
            append_csv_gz(runs_path, pd.DataFrame([r]), RUNS_COLS)
            if s:
                append_csv_gz(steps_path, pd.DataFrame(s), STEPS_COLS)
    else:
        with ProcessPoolExecutor(max_workers=n_jobs) as pool:
            futs = {pool.submit(_one_trial, p): p for p in payloads}
            for fut in as_completed(futs):
                r, s = fut.result()
                print(
                    f"  {r['cfg']} N={r['N']} a={r['alpha']:.4f} id={r['seed']} -> {r['status']}"
                )
                append_csv_gz(runs_path, pd.DataFrame([r]), RUNS_COLS)
                if s:
                    append_csv_gz(steps_path, pd.DataFrame(s), STEPS_COLS)

    print(f"wrote {runs_path}")
    print(f"wrote {outdir / 'sat_labels.csv.gz'}")
    if steps_path.is_file():
        print(f"wrote {steps_path}")
    print(f"wrote {outdir / 'manifest.json'}")
