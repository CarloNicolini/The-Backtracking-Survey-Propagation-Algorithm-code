"""Label each grid instance SAT or UNSAT with minisat."""

from __future__ import annotations

import re
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import pandas as pd

FORMULA_RE = re.compile(r"alpha=([\d.]+)-SAT_seed=(\d+)\.cnf$")
MINISAT_STATUS = {10: "SAT", 20: "UNSAT"}


def minisat_label(minisat: str, cnf: Path, timeout_s: int) -> str:
    try:
        proc = subprocess.run([minisat, str(cnf)], capture_output=True, timeout=timeout_s)
    except subprocess.TimeoutExpired:
        return "UNKNOWN"
    return MINISAT_STATUS.get(proc.returncode, "UNKNOWN")


def label_grid(data_dir: Path, minisat: str, timeout_s: int, jobs: int) -> pd.DataFrame:
    """Solve one formula per (alpha, seed); all configs share the instance of a seed."""
    formulas = {}
    for cnf in sorted(data_dir.glob("runs/*/Formula_CNF*.cnf")):
        m = FORMULA_RE.search(cnf.name)
        formulas.setdefault((float(m[1]), int(m[2])), cnf)
    keys = sorted(formulas)
    with ThreadPoolExecutor(jobs) as pool:
        sat = list(pool.map(lambda k: minisat_label(minisat, formulas[k], timeout_s), keys))
    labels = pd.DataFrame(keys, columns=["alpha", "seed"]).assign(sat=sat)
    out = data_dir / "sat_labels.csv.gz"
    labels.to_csv(out, index=False, compression="gzip")
    print(f"wrote {out}: {labels['sat'].value_counts().to_dict()}")
    return labels
