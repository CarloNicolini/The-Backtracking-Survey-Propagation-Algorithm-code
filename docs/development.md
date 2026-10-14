# Documentation and development

## Build the documentation

```bash
uv sync --locked --group docs
uv run --group docs sphinx-build -W --keep-going -b html docs docs/_build/html
```

The site uses PyData Sphinx Theme, MyST Markdown, and Sphinx autodoc. Its API
reference imports the installed native package so signatures track executable
code. Documentation source files and the notebook are included in the sdist.
Generated HTML is ignored by Git. The build needs no RandSATBench data and does
not run solver experiments.

## Run the Python checks

```bash
uv sync --locked --group test
uv run --group test pytest tests/python -q
```

CLI parity tests additionally need `BSP_TEST_EXECUTABLE=/absolute/path/to/bsp`.
Without it those comparisons are skipped. Tests cover native exceptions and
ownership, datasets and persistence, parameter cloning, process ordering and
cancellation. For C++ build/test instructions see the repository README.

## Validate the notebook

```bash
uv sync --locked --group notebooks
uv run --group notebooks python - <<'PY'
from pathlib import Path
import nbformat
from nbclient import NotebookClient
path = Path("notebooks/native_bsp_experiments.ipynb")
notebook = nbformat.read(path, as_version=4)
NotebookClient(notebook, timeout=300, kernel_name="python3").execute()
nbformat.write(notebook, path)
PY
```

Defaults use a small random ensemble and no persistence. This command refreshes
saved outputs. Run it with the project environment's kernel and restart kernels
after rebuilding a loaded extension. Reference-generation failures leave UNKNOWN
labels. They must not be silently interpreted as UNSAT or removed when comparing
unconditional solve rates.

The initial API supports complete runs and batch experiments. Future work includes
solver effort budgets, interactive stepping, independent ensemble/dense research
interfaces, and Windows support. These are not requirements for using the documented
Unix native interface.
