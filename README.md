# The-Backtracking-Survey-Propagation-Algorithm-code

This code solves random K-SAT instances using Survey Inspired Decimation (SID)
or Backtracking Survey Propagation (BSP).

If you want a faster version of this code, email: raffaele.marino@unifi.it
or marinoraffaele.nunziatella@gmail.com

For any problem, email: marinoraffaele.nunziatella@gmail.com

## Build (CMake)

Requires CMake ≥ 3.19 and a C++14 compiler (g++ on Linux, or AppleClang/Clang on
macOS). Network access is needed on the first configure so
CMake can FetchContent the cxxopts header library.

```bash
cmake --preset release
cmake --build --preset release -j
```

The executable is `build/release/bsp`. For a Debug build, use the `debug` preset
(`build/debug/bsp`).

### Options

| CMake option | Default | Meaning |
|---|---|---|
| `CMAKE_BUILD_TYPE` | set by preset | `Debug`, `Release`, `RelWithDebInfo`, or `MinSizeRel` |
| `BSP_NATIVE_ARCH` | `ON` | Optimize for the host CPU (`-march=native`). Turn **off** for portable binaries. |

Examples:

```bash
# Portable Release binary
cmake --preset release -DBSP_NATIVE_ARCH=OFF
cmake --build --preset release -j

# Debug build (symbols; useful for Valgrind)
cmake --preset debug -DBSP_NATIVE_ARCH=OFF
cmake --build --preset debug -j
```

Release builds enable strong optimization flags for GCC and Clang, including
link-time optimization (IPO/LTO) when supported.

Optional install:

```bash
cmake --install build/release --prefix /path/to/prefix
```

## Usage

Generate a random instance and solve it:

```bash
./build/release/bsp -w <K> <alpha> <N>
```

Load a CNF file and solve it:

```bash
./build/release/bsp -l <formula.cnf>
```

Example (3-SAT, clause density 4.0, 50 variables):

```bash
./build/release/bsp -w 3 4.0 50
```

## Thermodynamic Survey Propagation (ThermoSP)

The option `--cav-temp=T` deforms the SP cavity aggregation with a temperature
T. The default T = 0 keeps the hard SP factors and the current behavior. For
T > 0 the solver aggregates the cavity warning configurations with the Gibbs
weight e^(-min(p,q)/T). Here min(p,q) counts the conflicting warning pairs of
one configuration. The map T = 1/y relates the deformation to the
finite-energy SP(y) equations. For T > 0 the printed complexity is
Sigma_e = G + y e of the SP-y Bethe functional (`paper/sections/spy.tex`), and
at T -> 0 it equals the legacy complexity. A value T = 0.05 means y = 20, and
the Gibbs weight of one conflict is then e^-20: the deformation has an effect
only for y of order one, that is T between about 0.3 and 1.

```bash
./build/release/bsp --cav-temp=0.1 -w 3 4.0 50
```

The option `--act-temp=T` turns the Gibbs decimation policy on. The policy
draws the assignment of a variable with the weights e^(-Phi/T), where Phi is
the free energy of the direction. The default T = 0 keeps the hard sT>sF rule.
The scorer `--scorer=fth` ranks the variables by the absolute free-energy bias
B = Phi_minus - Phi_plus. Both options need `--cav-temp>0`.

```bash
./build/release/bsp --cav-temp=0.1 --act-temp=0.1 --scorer=fth -w 3 4.0 50
```

The option `--fe-backtrack` moves the backtracking step to free energies. The
solver releases the assignments with the largest free-energy cost
-T log I(k) first, where I(k) is the release score of
`--dynamic-i-backtrack`. Both rules compute I(k) from the messages of the
current fixed point. It
draws the move type with a Gibbs split between a release and a decimation. The
cost `--bt-cost=C` charges one back move in that split. Both need
`--cav-temp>0`. The default cost 0.4 is calibrated at `--cav-temp=0.05`
`--act-temp=0.02` on the golden instance: the back to decimation ratio there is
1.0 against 0.9 for the ratio rule. All free energies scale with
`--cav-temp`, so scale the cost with it. A zero cost can make the solver loop.

The unit test `build/release/bsp-test` holds the numeric identities of the deformation.
The script `tools/regression_thermo.sh` rebuilds the tree and compares three
golden solver runs stored in `tests/golden`. Numbers match within 1e-12
absolute and 1e-6 relative, and all other text matches exactly. Operate the
script after each change to the solver.

```bash
./tools/regression_thermo.sh
```

### 1RSB cluster reweighting

The option `--rsb-m=M` deforms the cluster measure with mu_m(C) proportional to
e^(m N s_C), where s_C is the internal entropy of the cluster C. SP messages
carry warnings only, and they do not carry s_C. The code therefore uses the
Maneva-Mossel-Wainwright closure s_C = ln2 q_C, where q_C is the fraction of
free variables of the cluster. Under this closure `--rsb-m=M` is the unfrozen
tilt of the next section with gamma = M ln2, and the two flags add up. The
exact measure needs the entropic 1RSB equations, with surveys over BP messages.
At m = 0 the solver keeps the uniform cluster measure.

A positive tilt drives the surveys toward the trivial fixed point. At N = 500
the solver then gives the whole formula to WalkSAT after a few steps, and a
solved instance there measures WalkSAT, not SP.

```bash
./build/release/bsp --rsb-m=0.5 -w 3 4.0 50
```

### Unfrozen-cluster bias

The option `--rsb-gamma=G` deforms the same star with mu proportional to
e^(gamma * q_C), where q_C is the fraction of variables that receive no warning.
Only the hard unfrozen mass A0*B0 is multiplied by e^gamma. The frozen sectors
stay as they are, and the finite-temperature balanced conflicts (p=q>0) are not
boosted. The clause message is still eta = pi_u / z. At gamma = 0 the factors
match ordinary SP. This flag is not `--scorer=gamma:<g>`, which only ranks
variables for decimation. With gamma != 0 the printed complexity is the free
entropy of the biased measure. The paramagnetic test, which hands the formula
to WalkSAT, uses the hard site factors 1-pi^+pi^-, because a warning-free
site has z = e^gamma under the tilt.

```bash
./build/release/bsp --rsb-gamma=0.5 -w 3 4.0 50
```

## Memory checking with Valgrind

Valgrind runs on **Linux** only (not available as a native macOS/Homebrew bottle).
Prefer a Debug build without native-arch tuning:

```bash
cmake --preset debug -DBSP_NATIVE_ARCH=OFF
cmake --build --preset debug -j

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
  ./build/debug/bsp -w 3 3.0 30
```

On macOS, the same check can be run in a Linux container, for example:

```bash
docker run --rm -v "$PWD":/src -w /src ubuntu:24.04 bash -lc '
  apt-get update -qq && apt-get install -y -qq g++ cmake make valgrind
  cmake --preset debug -DBSP_NATIVE_ARCH=OFF
  cmake --build --preset debug -j
  valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
    ./build/debug/bsp -w 3 3.0 30
'
```

## Native Python interface

`pybsp` calls the C++ `bsp_core` library through nanobind and returns NumPy
assignments, pandas diagnostic tables, and captured logs in memory.

```bash
uv sync --locked
uv run ipython
```

Python 3.12+ and GCC/Clang on macOS/Linux are required for source builds.
scikit-learn is included, with `KSATSolver` parameter inspection and cloning.
To use an existing environment, install with `uv pip install .` or
`python -m pip install .`.

```python
import pybsp

formula = pybsp.Formula([[1, -2, 3], [-1, 2, 3]], n=3)
result = pybsp.solve(formula, pybsp.SolverOptions(seed=1))
print(result.status, result.assignment)
print(result.steps.head())
```

A 2D integer array `(clauses, K)` represents one formula; a 3D array represents
a uniformly sized batch. Iterables of `Formula` objects or dataset records
support varying sizes. A failed heuristic run never proves UNSAT.

### Dataset pipelines and batch solving

```python
DATASET_ROOT = "~/workspace/RandSatBench/datasets/3SAT"
dataset = (
    pybsp.Dataset.from_randsatbench(DATASET_ROOT, split="train")
    .select(n=[16, 32], alpha=(4.0, 4.3))
    .sample(num_instances=10, by=["N", "alpha"], seed=123)
)
dataset.head().to_frame()

solver = pybsp.KSATSolver(r=0.9, random_state=42, n_jobs=2)
assignments = solver.fit_predict(dataset)
print(solver.run_table_.head())
print(solver.report_.metrics())
```

Operations return new datasets; formulas remain lazy. `sample(frac=0.1)` samples
a fraction, while `num_instances` selects a fixed count (per group with `by`).
Filtering, querying, sorting, shuffling, slicing, splitting and sharding are
supported. Reference and BSP assignments are independently verified; different
satisfying assignments are equally correct.

`solve_batch()` offers the functional interface and `iter_solve()` streams
results. Spawned workers use bounded dispatch; the parent displays progress and
coordinates cooperative cancellation. Use `fit_predict()` to solve once:
`fit(X).predict(X)` performs two searches without learning a transferable model.

### Random datasets and explicit persistence

```python
generated = pybsp.Dataset.from_random_ksat(
    n=32, alpha=4.0, k=3, num_instances=20, seed=123,
    solver=pybsp.SolverOptions(seed=99), show_progress=False,
)
# generated.save_to_disk("experiments/labelled-ksat")
# restored = pybsp.Dataset.load_from_disk("experiments/labelled-ksat")
```

`m` can specify the clause count instead of `alpha`. Omitting `solver` defers
reference solving. Portable snapshots preserve formulas, labels and generation
metadata independently of original CNF paths. Ordinary runs create no solver
files; explicit Minisat use may require cleaned temporary inputs.

### Notebooks, documentation, and building

```bash
uv sync --locked --group notebooks
uv run --group notebooks jupyter lab

uv sync --locked --group docs
uv run --group docs sphinx-build -W --keep-going -b html docs docs/_build/html
```

Open the [experiment notebook](notebooks/native_bsp_experiments.ipynb), which
runs a small generated-data experiment by default and supports RandSATBench as
an alternative. It demonstrates validation/held-out splits, parameter sweeps,
recovery curves, runtime plots, paired comparisons and explicit persistence.

The [PyData Sphinx documentation](docs/index.md) includes installation,
the [mathematical background](docs/sat-and-bsp.md), quickstarts, dataset
tutorials, experiments and a generated
[API reference](docs/api/index.rst). Open `docs/_build/html/index.html` after
building. The [detailed native guide](docs/python-native.md) retains option
mapping, diagnostic controls, logging and cancellation details.
The experiment tutorial includes interactive Plotly figures using the `seaborn`
template, including verified solve fraction versus clause density.

Packaging uses the PEP 517 `scikit_build_core.build` backend. uv lets
scikit-build-core provision build tools and compile the PIC `bsp_core` target,
then link/install `pybsp._native` into the wheel. Native Python builds use
Release settings with CPU-specific optimization and OpenMP disabled by default;
temporary CMake builds do not create source-tree IDE links.

```bash
uv build                              # sdist and compiled wheel in dist/
uv sync --reinstall-package pybsp     # Rebuild after C++/binding changes
# With an OpenMP runtime available:
# uv sync --reinstall-package pybsp -Ccmake.define.BSP_ENABLE_OPENMP=ON
```

The initial API supports complete native runs and batch experiments. General
effort budgets, interactive stepping and wrappers for the PRE paper's sparse
ensemble/dense Gaussian calculations remain separate work.
