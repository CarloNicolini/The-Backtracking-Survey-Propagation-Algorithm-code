# Solve and inspect a formula

A CNF formula is a conjunction of clauses, each containing a disjunction of
signed literals. A satisfying assignment makes every clause true. The
[mathematical introduction](sat-and-bsp.md) describes this representation and
the survey messages that guide BSP's assignment and backtracking moves.

## Represent inputs unambiguously

A two-dimensional integer matrix is **one** formula of shape `(clauses, K)`.
A three-dimensional array is a batch of equal-sized formulas. For ragged CNFs
or unused variables, construct `Formula` objects explicitly.

```python
import numpy as np
import pybsp

formula = pybsp.Formula(np.array([[1, -2, 3], [-1, 2, 3]], dtype=np.int32), n=4)
# Variable-length clauses:
formula = pybsp.Formula([[1], [-1, 2], [3, -4]], n=4)
# Other constructors:
formula = pybsp.Formula.from_dimacs("p cnf 2 2\n1 0\n-1 2 0\n")
# formula = pybsp.Formula.from_file("instance.cnf")
```

Literals are signed one-based variable indices. Zero terminates a DIMACS clause
but is never an array/list literal. Repeated variables in one clause are rejected.
The native limits are one million variables and 500 literals per clause.

## Native results

```python
result = pybsp.solve(formula, pybsp.SolverOptions(seed=1))
assert result.assignment is None or formula.is_satisfied(result.assignment)
result.steps.head()
result.moves.head()
result.stdout
```

Assignments are signed literals in variable order. `solutions` has shape
`(number_of_solutions, number_of_variables)`. `surveys` has columns `sT, sF, sI`.
Arrays own their storage and are read-only. Diagnostic tables are pandas frames.
Optional variable, residual and population histories are disabled by default. See the [detailed guide](python-native.md#options) for enabling and sampling them.

| Outcome | Meaning |
|---|---|
| `solved` | A satisfying assignment was verified |
| `failed` | The heuristic search was unsuccessful. UNSAT is not established |
| `cancelled` | Cooperative interruption, with available partial diagnostics |
| `SolverError` | Execution failure. `.result` holds captured output/diagnostics |

Invalid inputs/options raise Python exceptions. Logs are captured. An optional
`logger=` forwards them after the native run. Native runs release the GIL but
serialize within one process because the C++ state is global.

## Batch solving

```python
solver = pybsp.KSATSolver(r=0.9, random_state=42, n_jobs=2)
assignments = solver.fit_predict([
    pybsp.Formula([[1]], n=3),
    pybsp.Formula([[1], [-1]], n=3),
])
solver.run_table_
solver.report_.metrics()
```

There is one assignment or `None` per returned row. `fit_predict()` replaces
previous outputs. Constructor parameters follow sklearn conventions and support
cloning, `get_params()` and `set_params()`. The solver is transductive: each
call searches on its supplied formulas. Consequently `fit(X).predict(X)`
performs two searches, and `score(X)` runs a new batch.
Use `evaluate(X)` to solve once, then compute several metrics from the report.

`solve_batch()` is the functional equivalent. `iter_solve()` streams bounded
chunks of output in completion order. Close the iterator when stopping early.
`solve_batch()` restores input order. A supplied `CancellationToken` coordinates
cooperative stopping across spawned workers. Full results are retained only with
`keep_results=True`. Collected assignments still consume memory proportional
to the batch size.

## Explicit persistence

```python
# Destination must be new or empty:
# result.export("experiments/one-run")
```

Ordinary runs do not write solver files or change working directory. An
explicitly enabled external Minisat oracle may use cleaned temporary inputs.
For portable dataset snapshots, see [datasets](datasets.md).
