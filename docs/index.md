# pybsp: native SAT solving in Python

Run Backtracking Survey Propagation directly from Python, inspect NumPy
assignments and pandas diagnostics, and organize reproducible batch experiments.
The native extension links the existing C++ core. Ordinary runs retain their
inputs, measurements and logs in memory.

```python
import pybsp

formula = pybsp.Formula([[1, -2, 3], [-1, 2, 3]], n=3)
result = pybsp.solve(formula, pybsp.SolverOptions(seed=1))
print(result.status, result.assignment)
```

The [mathematical introduction](sat-and-bsp.md) connects CNF formulas and factor
graphs to survey propagation and BSP. Start with [installation](installation.md)
and the [quickstart](quickstart.md).
Use [dataset pipelines](datasets.md) to select or generate instances, and the
[experiment tutorial](experiments.md) to compare configurations and visualize
results. The [API reference](api/index.rst) is generated from the installed package.

This is the initial 0.1 API for complete runs. It provides verified SAT solutions. Heuristic failure is not an UNSAT certificate. Solver-wide effort budgets and
interactive stepping are not exposed. The PRE paper's ensemble and dense-model
calculations remain separate research workflows.

```{toctree}
:maxdepth: 2
:caption: Documentation

user-guide
api/index
development
```
