# Native BSP from Python and notebooks

Install from the repository with `python -m pip install .`. The PEP 517 build
uses scikit-build-core and nanobind, builds `bsp_core` as position-independent
code, and statically links it into `pybsp._native`. No `bsp` executable or
separate `libbsp_core` installation is needed. A GCC/Clang compiler is needed
to build from source. Scikit-build-core provisions CMake/Ninja when needed.
An installed wheel needs no compiler. Python
3.12+ on Unix platforms is the initial target. Standard CMake builds use
OpenMP when available. Python wheels default to serial loops to avoid requiring
a separately installed OpenMP runtime. Enable it in a local Python build with
`pip install . -Ccmake.define.BSP_ENABLE_OPENMP=ON`.
Wheel builds disable `-march=native` and do not create source-tree CMake links.

With uv, use `uv sync --locked` to install the project in editable mode and
`uv run python` (or `uv run ipython`) to use that environment. `uv build` creates
an sdist and wheel through the same isolated scikit-build-core/CMake backend.
After C++ changes, rebuild with `uv sync --reinstall-package pybsp`.
See the [installation guide](installation.md) for the complete uv
installation and build workflow.

```python
import pybsp

formula = pybsp.Formula.random(k=3, alpha=4.0, n=500, seed=1)
options = pybsp.SolverOptions(r=0.9, variable_history=True, diag_every=10)
result = pybsp.solve(formula, options)

result.status          # 'solved', 'failed', or 'cancelled'
result.assignment      # first verified solution: signed literals, shape (N,)
result.solutions       # all verified solutions: NumPy int32, shape (S, N)
result.surveys         # NumPy float64: sT, sF, sI; (N, 3) after SP initialization
result.steps           # pandas DataFrame, one row per SP fixed point
result.moves           # pandas DataFrame, decimation/backtracking/UP moves
result.variables       # optional sampled variable history
result.rsb2            # optional stability/population observables
result.sp2             # optional finite-mu 2RSB observables
result.whitening       # whitening summary for each verified solution
result.stdout          # captured BSP and WalkSAT standard output
result.stderr          # captured warnings/errors
result.logs            # typed records: level, channel, message

assert result.assignment is None or formula.is_satisfied(result.assignment)
result.export("my-run") # explicit persistence; destination must be new or empty
```

`solve()` calls C++ directly. It does not launch the BSP executable, parse its
logs for scientific results, change the working directory, or write solver
files. The C++ run API is also used by the existing executable. The legacy
`run_trial()` and `bsp-grid`, `bsp-plot`, and `bsp-label` remain available.

## Input and reproducibility

```python
import numpy as np

formula = pybsp.Formula(np.array([[1, -2, 3], [-1, 2, -4]], dtype=np.int32), n=5)
formula = pybsp.Formula([[1], [-1, 2], [3, 4, -5]], n=6)
formula = pybsp.Formula.from_dimacs("p cnf 2 2\n1 0\n-1 2 0\n")
formula = pybsp.Formula.from_file("input.cnf")
```

Literals use DIMACS numbering: `+i` means variable i, `-i` its negation. Zeros
are clause terminators only in DIMACS text. For arrays/lists, every entry must
be a nonzero integer. `n` defaults to the largest literal index. Provide it to
retain unused variables or represent a formula with no clauses. Empty clauses
are accepted and produce a failed run. Repeated variables within a clause are
rejected because the legacy graph assumes a single incidence per variable.
The current WalkSAT implementation limits N to 1,000,000 and clause length to
500. No dense clause-by-variable matrix is created.

`Formula.random()` retains the actual seed, even when no seed is supplied.
`SolverOptions.seed=None` inherits this seed. For other formulas it generates
a fresh seed. A random formula run replays generation to preserve the CLI's
RNG position. Setting a different solver seed solves the same stored clauses
with fresh message initialization. Loading a random formula from DIMACS uses
the CLI's load semantics. It does not replay random generation. Determinism is
checked against the CLI on the same build/platform, not guaranteed across
platforms or different floating-point/OpenMP configurations.

## Options

Every implemented solver switch is represented in `SolverOptions`. Hyphens
become underscores. Defaults follow the C++ executable:

| CLI controls | Python options |
|---|---|
| `--scorer`, `--r`, `--seed`, `--theta`, `--veto`, `--eps`, `--damping` | `scorer`, `r`, `seed`, `theta`, `veto`, `eps`, `damping` |
| `--cav-temp`, `--act-temp`, `--rsb-m`, `--rsb-gamma`, `--tsallis-kappa` | `cav_temp`, `act_temp`, `rsb_m`, `rsb_gamma`, `tsallis_kappa` |
| `--minisat`, `--oracle-timeout`, `--oracle-dir`, `--oracle-pick` | `minisat`, `oracle_timeout`, `oracle_dir`, `oracle_pick` |
| `--lookahead`, `--corr-batch`, `--adaptive-r`, `--dynamic-i-backtrack` | `lookahead`, `corr_batch`, `adaptive_r`, `dynamic_i_backtrack` |
| `--fe-backtrack`, `--bt-cost`, `--frac` | `fe_backtrack`, `bt_cost`, `frac` |
| `--rsb2-stab`, `--rsb2-x`, `--rsb2-pop`, `--rsb2-every` | `rsb2_stab`, `rsb2_x`, `rsb2_pop`, `rsb2_every` |
| `--sp2-mu2`, `--sp2-x`, `--sp2-pop`, `--sp2-sweeps`, `--sp2-every` | `sp2_mu2`, `sp2_x`, `sp2_pop`, `sp2_sweeps`, `sp2_every` |
| `--quiet`, `--verbose`, `--log-level`, `--log-prefix` | `log_level='warn'/'debug'/'trace'/...`, `log_prefix` |
| `--diag`, `--diag-every`, `--dump-residuals` | steps/moves always collected. `variable_history`, `diag_every`, `residual_history` |
| `--outdir` | `result.export(directory)` |

`scorer` accepts `cert`, `pol`, `i_c`, `fth`, `rsb`, `gamma:<g>`, or
`soft_cert:<beta>`. Native parameters must be finite and satisfy the CLI's
range/combination constraints. `oracle_pick>0` remains explicitly unsupported.
Zero disables `rsb2_x` and `sp2_mu2`. `rsb2_every=0` means 1 as in the CLI;
`diag_every` must be positive. `sp2_every=0` runs its diagnostic once.

Steps and moves are collected by default. Large histories require opt-in:

```python
options = pybsp.SolverOptions(
    rsb2_x=0.5, rsb2_pop=64, population_history=True,
    variable_history=True, residual_history=True, diag_every=10,
)
```

`residuals` maps sampled step numbers to `Formula` objects. `populations` maps
sampled steps to owned arrays with one population per active edge. The
`population_edges` table identifies each row by the original clause number,
variable number, and literal sign. Population sampling also respects the
chosen diagnostic's `rsb2_every`/`sp2_every` interval. Optional histories can
be large. They remain in memory until released or explicitly exported.

All arrays own detached storage and are read-only by default. They remain
valid after their result object is deleted. Surveys are the last available
SP values, or shape (0, 3) if SP was never initialized, including stale values for variables already fixed. Consult the
variable history's `fixed` column when interpreting them.

## Logs, errors, and interruption

```python
import logging
result = pybsp.solve(formula, logger=logging.getLogger("bsp"))
```

Logs are captured during C++ computation and forwarded to the logger after
the run. Prefixes affect captured text. Structured records retain the level
and stdout/stderr channel independently. WalkSAT's historical console output
is captured too. External Minisat's own output is suppressed, as in the CLI.

Invalid inputs/options raise `ValueError` or `TypeError`. Heuristic failure,
negative complexity, non-convergence, or a contradiction reached during the
run produce `status='failed'` with a reason and partial diagnostics. This is
not a proof that the input is UNSAT. Internal execution failures raise
`pybsp.SolverError`. Its `.result` retains the captured output and diagnostics.

Native runs release the GIL and are serialized because solver/WalkSAT globals
are shared. Notebook interruption (KeyboardInterrupt) is checked at safe
boundaries and returns `status='cancelled'`. Cancellation from another thread
is also supported:

```python
from concurrent.futures import ThreadPoolExecutor

token = pybsp.CancellationToken()
with ThreadPoolExecutor(max_workers=1) as pool:
    future = pool.submit(pybsp.solve, formula, options, cancel=token)
    token.cancel()
    result = future.result()
```

Cancellation is cooperative. Checks occur
between SP sweeps, population sweeps, oracle/probe polls, and blocks of WalkSAT
flips. The existing lookahead policy may fork native probe children. It never
launches a BSP subprocess or writes probe files. An explicitly enabled
Minisat oracle spawns Minisat with a uniquely named temporary CNF input,
removed on success, failure, timeout, or cancellation. No process-wide alarm
or signal handler is installed by the wrapper.

## Validation commands

```sh
uv sync --locked --group test
uv run --group test pytest tests/python/test_native.py
BSP_TEST_EXECUTABLE=/absolute/path/to/bsp uv run --group test pytest tests/python/test_native.py
```

The second test mode also compares native diagnostics and solutions against
CLI runs with fixed seeds, random/generated and loaded inputs, thermodynamic
settings, Tsallis deformation, 2RSB diagnostics, and lookahead controls.

## Labelled dataset analysis (RandSATBench)

`SatDataset` reads a RandSATBench SAT split, and `analyze_dataset()` evaluates
it with the native solver. CNFs are loaded one at a time. Ordinary evaluation
keeps scalar comparisons in memory and writes no files. This supports the
`N*_M*_id*.cnf` naming convention and CSV columns `cnf_file,sat,assignment`.
Assignments contain signed variable numbers with an optional final `0`.

```python
import pybsp

dataset = pybsp.SatDataset.from_randsatbench(
    "~/workspace/RandSatBench/datasets/3SAT", split="test",
)
print(dataset.instances.head())

# Start with a subset; omit select() to evaluate the whole split.
subset = dataset.select(n=[16, 32, 64], alpha=(3.8, 4.6), limit=100)
report = pybsp.analyze_dataset(
    subset,
    pybsp.SolverOptions(r=0.9),
    seeds=[1, 2, 3],
)
report.runs                       # one row per instance and solver seed
report.summary()                  # grouped by N and alpha
report.summary(by=["N"])          # aggregate across clause densities
report.summary(by=())             # overall summary
```

Pass the dataset root or its `train`/`test` directory. Labels are discovered
beside the root/split, or under the repository's `3SAT/` or `4SAT/` directory.
This accommodates the local layout with CNFs under `datasets/3SAT/test` and
labels in `3SAT/test_labels.csv`. Label paths such as
`test-final/N16_M48_id1.cnf` match by filename **within the selected split**.
Use `labels_csv="/path/to/test_labels.csv"` to override discovery. Missing
labels remain `UNKNOWN`. CSV codes `1`, `0`, and `-1` mean SAT, UNSAT, and
UNKNOWN respectively (blank status also means UNKNOWN).

Every SAT reference assignment is checked against its original clauses.
BSP solutions are checked independently. `report.runs` includes solver
status/reason/timing, reference validity/errors, number of solutions, step/move
counts, and `hamming_distance`, `agreement`, and `exact_match` against the first
BSP solution. Different satisfying assignments count equally as successful. Matching CaDiCal's particular assignment is not a requirement. A verified BSP
solution on a reference-UNSAT instance sets `label_conflict=True` for inspection.
Failed BSP runs remain heuristic failures and are never UNSAT predictions.

Summary `sat_recovery` is the fraction of distinct instances with a verified
SAT reference that BSP solves on **at least one** requested seed. Invalid or
missing references are excluded from that denominator and reported separately. If no valid SAT references exist, recovery is NaN. `solved_runs` counts successful
trials, while `solved_instances` counts distinct successful instances, including
those with unknown labels. Timing measures native solver time, excluding CNF
loading and reference validation.

Explicit native solver trajectory seeds must be positive. Dataset generation
seeds may be zero. `analyze_dataset()` defaults to seed 1 when no options are given. Otherwise it
uses `options.seed`. Supplying `seeds` overrides that seed for each trial.
Invalid options raise before evaluation. CNF loading and native execution errors
are recorded as `status='error'` per run so other instances can still be analyzed.
A `CancellationToken` or notebook interruption stops evaluation and returns
completed rows in a partial report with `report.cancelled=True`.
`report.total_instances` records the selected instance count before cancellation.

```python
# Retaining complete results is opt-in: this can consume substantial memory.
report = pybsp.analyze_dataset(subset, seeds=[1], keep_results=True)
name = report.runs.iloc[0]["name"]
result = report.results[(name, 0)]  # (CNF filename, zero-based trial index)
result.steps
result.stdout

# Progress runs on the Python side after every trial.
report = pybsp.analyze_dataset(
    subset,
    progress=lambda done, total, row: print(done, "/", total, row["status"]),
)

# Persist only when requested, using ordinary pandas exports.
report.runs.to_csv("bsp-dataset-runs.csv", index=False)
report.summary().to_csv("bsp-dataset-summary.csv", index=False)
```

Full native results, captured logs and histories are released between runs by
default (`report.results` is empty). Set `keep_results=True` to retain them,
and use the usual `SolverOptions` to enable sampled histories. Optional logger
forwarding is available through `logger=...`. The callback receives
`progress(completed_runs, total_runs, row)`. A tqdm progress bar is enabled by
default, adapting to notebooks and terminals. It counts instance-seed trials
and displays elapsed time, estimated remaining time, speed, and the latest
status. It closes on completion, cancellation, or an exception, preserving
the completed count for partial reports. Use `show_progress=False` to silence
the bar. Custom progress callbacks still run. For notebook widget bars, install
`ipywidgets` in the notebook environment. A text bar works without it.
Native evaluation remains serial
because the underlying solver state is global. The legacy experiment runner
and dataset indexing helpers remain available.

## Batch solver and scikit-learn parameter interface

`KSATSolver` uses scikit-learn's `BaseEstimator` for parameter inspection,
modification, and cloning. scikit-learn is included in the current runtime
dependencies. Install the project with:

```bash
uv sync --locked
# Or: python -m pip install .
```

```python
import pybsp

solver = pybsp.KSATSolver(r=0.9, random_state=42, n_jobs=4, show_progress=True)
dataset = pybsp.Dataset.from_randsatbench(
    "~/workspace/RandSatBench/datasets/3SAT",
).select(n=64, alpha=(4.0, 4.3), limit=100)

assignments = solver.fit_predict(dataset)
solver.assignments_       # signed NumPy arrays, or None for unsuccessful runs
solver.statuses_          # solved / failed / cancelled / error
solver.run_table_         # pandas table; reference checks and solver measurements
solver.report_.metrics()  # solve rate, best-of-seeds SAT recovery, timing, errors
solver.report_.summary()  # grouped instance-level recovery
solver.get_params()
solver.set_params(r=0.8)
```

All native solver parameters are constructor keywords. `random_state` replaces
`SolverOptions.seed` and accepts an integer or None. Explicit integer seeds
produce the same per-input trajectories regardless of worker scheduling.
The constructor only stores parameters. Validation occurs at execution time.
`fit_predict()` solves once and replaces previous retained outputs. `fit()`
solves its inputs and returns the solver. `predict()` performs a new search
and replaces the outputs. Consequently `fit(X).predict(X)` runs two searches.
No model is learned across formulas. The adapter supports parameter utilities,
not ordinary classification semantics or arbitrary sklearn feature pipelines.
`y=None` is accepted for interoperability and is never used to guide search. Reference comparisons use the labels in a Dataset.

A two-dimensional integer matrix `(n_clauses, K)` is **one formula**. A
three-dimensional integer array `(n_instances, n_clauses, K)` is a batch of
uniformly sized formulas. Other batches are iterables of `Formula` objects,
2D arrays, CNF paths, or lazy `DatasetInstance` records. Use `Formula(..., n=N)`
to preserve unused variables or to express variable-length clauses. Plain lists
of clauses should be wrapped in `Formula` to avoid ambiguous batch boundaries.

```python
import numpy as np

X = np.array([[[1, -2, 3], [-1, 2, 3]],
              [[1, 2, -3], [-1, -2, 3]]], dtype=np.int32)
assignments = solver.fit_predict(X)

# Functional interface; one row/assignment per trial.
report = pybsp.solve_batch(dataset, pybsp.SolverOptions(seed=42),
                          seeds=[1, 2, 3], n_jobs=4, batch_size=16)
report.to_frame()
report.manifest  # options, actual seeds, package/Python/platform, execution settings
```

`Dataset` is an alias for `SatDataset`. It supports `len(dataset)`, positional
indexing/slicing, iteration, and `iter_batches(batch_size)`. Its items are lazy file, generated-formula or snapshot records;
`item.load()` obtains a formula explicitly. `dataset.to_frame()` returns
a copy of metadata and label availability without loading CNFs. Set
`include_assignments=True` to include the original reference assignment text.
`report.to_frame()` returns a copy of its measurements.

Parallel execution uses spawned Python worker processes that call the native
extension, with at most two pending chunks per worker. `n_jobs=-1` uses the
reported CPU count. `batch_size` counts instance-seed trials. Full native results
are opt-in (`keep_results=True`), keyed by `(input_index, trial)`. Each assignment
matches the corresponding report row, and collected results are restored to
input order. Progress is displayed by the parent. Python scripts invoking
multiprocessing should place execution under `if __name__ == "__main__":`. Notebook cells can call the imported functions directly. OpenMP-enabled builds
should limit threads per worker to avoid oversubscription.

For bounded output memory, consume results incrementally:

```python
for row, assignment, result in pybsp.iter_solve(dataset, n_jobs=4):
    # Process or explicitly persist this row; do not retain all assignments.
    print(row["input_index"], row["status"])
```

Streamed results arrive in completion order, identified by `input_index` and
`trial`. `result` is None unless full retention was requested. Close the iterator
when ending iteration early. A supplied `CancellationToken` is bridged to
process-local native tokens. `solve_batch()` returns a partial report after a
notebook interruption or cancellation. Cancellation remains cooperative.

`solver.evaluate(dataset)` solves once and returns a report from which multiple
metrics can be computed without further searches. `solver.score(X)` runs a new
batch and returns the verified solved fraction over **all** inputs, including
UNSAT-labelled formulas. `sat_recovery` excludes invalid/missing SAT references
and counts an instance as recovered if any requested trial succeeds. Hamming
Clause verification determines correctness. Agreement measures similarity to a reference assignment. Use `ParameterGrid`
with the same instance subset and solver seeds to compare configurations. Reserve separate formulas for final evaluation after tuning.

This batch layer does not add wall-clock/work budgets or change BSP's numerical
policies. The PRE paper's sparse ensemble population dynamics and dense Gaussian
calculations remain separate research modules under
`papers/survey-propagation-2rsb/`. Finite-formula `sp2` measurements do not replace
those computations or constitute a new decimation policy.

## Dataset pipelines, generation, and snapshots

Dataset methods return new datasets, so filtering, sampling and inspection can
be chained without modifying the underlying DataFrame:

```python
dataset = (
    pybsp.Dataset.from_randsatbench(DATASET_ROOT, split="test")
    .select(n=[16, 32, 64], alpha=(4.0, 4.3))
    .sample(num_instances=10, by=["N", "alpha"], seed=123)
    .sort(["N", "alpha", "name"])
)
dataset.head().to_frame()
```

`sample(frac=0.1, seed=123)` takes a random fraction of the entire dataset.
`sample(num_instances=100, seed=123)` takes a fixed total count. Supply exactly
one of these. With `by="N"` or `by=["N", "alpha"]`, the count/fraction applies
separately within each group. Sampling is without replacement. A requested
count larger than a group raises an error, preserving the requested sampling balance. The source dataset is unchanged.

Other useful operations are:

```python
dataset.filter(lambda row: row["reference_status"] == "SAT")
dataset.query("N == 64 and alpha >= 4.1")
dataset.head(5)
dataset.tail(5)
dataset.shuffle(seed=123)
dataset.select([0, 2, 4])
dataset.shard(num_shards=4, index=0)
splits = dataset.train_test_split(test_size=0.2, seed=123)
dataset.pipe(lambda ds: ds.head(10))
```

`filter` evaluates a metadata dictionary without loading the formula. Filtering and
sorting evaluate metadata immediately. CNF loading and random formula generation
remain lazy. No automatic caching or Arrow/Hugging Face dependency is introduced.
`select(indices)` is positional. The existing `select(n=..., alpha=..., limit=...)`
remains supported. `train_test_split` creates disjoint evaluation subsets and
preserves original label/source split metadata. `num_rows`, `column_names`, and
`shape` describe the metadata view returned by `to_frame()`.

Random datasets store generation specifications and independent deterministic
formula seeds, producing formulas only when an item is loaded or solved:

```python
random_dataset = pybsp.Dataset.from_random_ksat(
    n=64, alpha=4.0, k=3, num_instances=100, seed=123,
)
# Alternatively m=256 specifies an exact clause count instead of alpha.

labelled = random_dataset.with_solutions(
    pybsp.SolverOptions(r=0.9, seed=42), n_jobs=4,
)
# Or supply solver=... directly to from_random_ksat().
labelled.head().to_frame()
```

`m` means clauses, while `n` means variables. `alpha` implies `ceil(alpha*n)`
clauses. Specifying both `m` and `alpha` requires agreement. The dataset's `alpha`
column reports the actual `m/n`. `seed` controls formula generation, independently
of reference solver seeds. With no solver, labels initially remain UNKNOWN.
Solving references is an explicit computation and loads/generates each formula.

Reference solvers can be `SolverOptions`, a configured `KSATSolver` (using that
object's execution settings), or a Python callable `Formula -> assignment/None`.
Callables currently run serially. Every returned assignment must satisfy the
original clauses. Invalid assignments raise. Successful assignments become SAT
references. Failed, cancelled, or erroneous native runs never become UNSAT
labels. Existing references are preserved when no new solution is available.
`reference_solver_status` records per-instance outcomes and `dataset.info`
records generation and reference solver provenance.

Explicit persistence snapshots both formulas and references into three files,
so a saved dataset remains usable after moving the directory or removing its
original CNF files:

```python
labelled.save_to_disk("experiments/labelled-ksat")
restored = pybsp.Dataset.load_from_disk("experiments/labelled-ksat")
restored.sample(frac=0.1, seed=123).head().to_frame()
```

The destination must be new or empty. `dataset.json` holds format/provenance,
`metadata.json` holds the indexed table, and `formulas.jsonl` holds the formula
snapshots. Loading reads metadata. Each formula is accessed by its stored file
offset when needed. Random formula seeds and RNG replay metadata are preserved.
This is the portable pybsp snapshot format described above.
Use `from_randsatbench()` for an existing RandSatBench layout. Ordinary metadata
pipelines and native solving create no dataset files unless saving is requested.
