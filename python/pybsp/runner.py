"""Run one bsp trial via subprocess."""

from __future__ import annotations

import json
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


def write_trial_manifest(
    path: Path,
    *,
    cmd: list[str],
    K: int,
    alpha: float,
    N: int,
    seed: int,
    flags: list[str],
    rundir: Path,
    timeout_s: float,
    returncode: int,
    wall_s: float,
    timed_out: bool,
    cnf: Path | None = None,
) -> None:
    payload = {
        "kind": "trial",
        "rundir": str(rundir.resolve()),
        "K": K,
        "alpha": alpha,
        "N": N,
        "seed": seed,
        "flags": flags,
        "timeout_s": timeout_s,
        "cmd": cmd,
        "returncode": returncode,
        "wall_s": wall_s,
        "timed_out": timed_out,
        "cnf": None if cnf is None else str(cnf.resolve()),
        "timestamp_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
    }
    path.write_text(json.dumps(payload, indent=2) + "\n")


def run_trial(
    bsp: Path,
    *,
    K: int,
    alpha: float,
    N: int,
    seed: int,
    flags: list[str],
    rundir: Path,
    timeout_s: float,
    cnf: Path | None = None,
) -> tuple[str, int, float, bool]:
    """Return (log_text, returncode, wall_s, timed_out).

    With cnf=None the binary builds a random instance (-w). With a CNF path it
    loads that file (-l). Outputs land in rundir via cwd and --outdir=.
    """
    rundir.mkdir(parents=True, exist_ok=True)
    log_path = rundir / "log.txt"
    cmd = [
        str(bsp),
        f"--seed={seed}",
        *flags,
        "--outdir=.",
        "--diag=t",
        "--diag-every=1000000",
    ]
    if cnf is None:
        cmd.extend(["-w", str(K), str(alpha), str(N)])
    else:
        cmd.extend(["-l", str(cnf.resolve())])
    t0 = time.perf_counter()
    timed_out = False
    try:
        proc = subprocess.run(
            cmd,
            cwd=rundir,
            capture_output=True,
            text=True,
            timeout=timeout_s,
        )
        rc = proc.returncode
        text = (proc.stdout or "") + (proc.stderr or "")
    except subprocess.TimeoutExpired as e:
        timed_out = True
        rc = 124
        out = e.stdout.decode() if isinstance(e.stdout, bytes) else (e.stdout or "")
        err = e.stderr.decode() if isinstance(e.stderr, bytes) else (e.stderr or "")
        text = out + err
    wall_s = time.perf_counter() - t0
    log_path.write_text(text)
    write_trial_manifest(
        rundir / "pybsp_manifest.json",
        cmd=cmd,
        K=K,
        alpha=alpha,
        N=N,
        seed=seed,
        flags=flags,
        rundir=rundir,
        timeout_s=timeout_s,
        returncode=rc,
        wall_s=wall_s,
        timed_out=timed_out,
        cnf=cnf,
    )
    return text, rc, wall_s, timed_out
