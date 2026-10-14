"""Experiment helpers for Backtracking Survey Propagation."""

__version__ = "0.1.0"

# Existing analysis/CLI users can import pybsp without loading the extension.
__all__ = [
    "Formula",
    "SolverOptions",
    "SolveResult",
    "SolverError",
    "LogRecord",
    "CancellationToken",
    "solve",
    "SatDataset",
    "Dataset",
    "DatasetInstance",
    "DatasetAnalysis",
    "analyze_dataset",
    "BatchResult",
    "solve_batch",
    "iter_solve",
    "KSATSolver",
]

from typing import TYPE_CHECKING


def __getattr__(name: str):
    if name in __all__:
        if name == "Dataset":
            from .benchmark import SatDataset

            return SatDataset
        if name == "KSATSolver":
            from . import solver as source
        elif name in {"BatchResult", "solve_batch", "iter_solve"}:
            from . import batch as source
        elif name in {
            "SatDataset",
            "DatasetInstance",
            "DatasetAnalysis",
            "analyze_dataset",
        }:
            from . import benchmark as source
        else:
            from . import native as source
        value = getattr(source, name)
        globals()[name] = value
        return value
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")


# Resolve the lazy public exports for static type checkers.
if TYPE_CHECKING:
    from .batch import BatchResult, iter_solve, solve_batch
    from .benchmark import DatasetInstance, SatDataset as Dataset
    from .solver import KSATSolver
    from .benchmark import DatasetAnalysis, SatDataset, analyze_dataset
    from .native import (
        CancellationToken,
        Formula,
        LogRecord,
        SolveResult,
        SolverError,
        SolverOptions,
        solve,
    )
