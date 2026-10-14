# Installation and native builds

## Install with uv

From a checkout of this repository:

```bash
uv sync --locked
uv run python -c "import pybsp; print(pybsp.__version__)"
```

Use `uv sync --locked --no-dev` for runtime dependencies alone. scikit-learn
is included in the current runtime dependencies, so `KSATSolver` is available
without an extra installation. The `sklearn` extra remains for compatibility
with earlier installation instructions.

To use an existing environment, run `uv pip install .`, or install through pip:

```bash
python -m pip install .
```

Python 3.12+ and GCC/Clang on macOS or Linux are required for source builds.
An installed wheel needs no compiler or separately installed BSP executable.
Windows support requires additional work.

## Compilation through scikit-build-core

`pyproject.toml` declares the PEP 517 backend `scikit_build_core.build`, with
scikit-build-core and nanobind as isolated build dependencies. The backend
provisions CMake/Ninja when needed, compiles the position-independent static
`bsp_core` target, and installs `pybsp._native` into the wheel alongside Python
sources. Build tools are not runtime imports.

```bash
uv build
```

This produces an sdist and a compiled wheel under `dist/`. Python builds use
Release settings, disable host CPU tuning and OpenMP by default, and use a
temporary CMake build directory. They do not build the CLI/tests or create the
source-tree IDE link. No separate shared `bsp_core` library is required.

Editable installations pick up Python changes. Recompile after C++ edits with:

```bash
uv sync --reinstall-package pybsp
```

With an OpenMP runtime available, explicitly enable it for a local build:

```bash
uv sync --reinstall-package pybsp -Ccmake.define.BSP_ENABLE_OPENMP=ON
```

## Jupyter environment

```bash
uv sync --locked --group notebooks
uv run --group notebooks jupyter lab
```

Choose the project's `.venv` Python as the kernel. The notebooks group includes
JupyterLab, ipykernel and ipywidgets for notebook progress bars. Download the
[experiment notebook](experiments.md) or open `notebooks/native_bsp_experiments.ipynb`
in the checkout. In notebook cells, call the imported batch functions directly.
Standalone scripts using multiple workers must guard execution with
`if __name__ == "__main__":`.

## Build this documentation

```bash
uv sync --locked --group docs
uv run --group docs sphinx-build -W --keep-going -b html docs docs/_build/html
```

Open `docs/_build/html/index.html` in a browser. The build imports the installed
native package for its API reference. It does not mock the extension or execute
long-running experiments. For development and testing, see [development](development.md).
