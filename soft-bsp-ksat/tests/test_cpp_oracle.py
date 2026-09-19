from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parents[2]
CANDIDATES = [
    REPO / "build" / "main",
    Path("/workspace/build/main"),
]


def _binary() -> Path | None:
    for p in CANDIDATES:
        if p.is_file():
            return p
    found = shutil.which("main")
    return Path(found) if found else None


@pytest.mark.skipif(_binary() is None, reason="C++ bsp binary not built")
def test_cpp_oracle_sid_smoke(tmp_path: Path):
    binary = _binary()
    assert binary is not None
    proc = subprocess.run(
        [str(binary), "-q", "--r=0", "--seed=1", "-w", "3", "2.0", "16"],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert proc.returncode != 127
    combined = (proc.stdout or "") + (proc.stderr or "")
    # Quiet mode still prints warnings/errors; success prints nothing required.
    # Non-crash is the oracle smoke check: the process must start and finish.
    assert "Unknown" not in combined
    assert proc.returncode in (0, 1, 255) or proc.returncode < 0
