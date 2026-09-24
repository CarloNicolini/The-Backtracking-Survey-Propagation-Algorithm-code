"""Typer CLI: bsp-grid, bsp-plot and bsp-label."""

from __future__ import annotations

from pathlib import Path
from typing import Optional

import typer

from pybsp.grid import run_grid
from pybsp.label import label_grid
from pybsp.plots import make_figures


def grid(
    grid: Path = typer.Argument(..., exists=True, dir_okay=False, help="JSON grid file"),
    outdir: Path = typer.Option(
        ...,
        "--outdir",
        "-o",
        help="Output directory for runs.csv.gz, steps.csv.gz, runs/",
    ),
    jobs: Optional[int] = typer.Option(
        None,
        "--jobs",
        "-j",
        help="Parallel trials (default: jobs field in JSON, else 1)",
    ),
) -> None:
    """Run a BSP experiment grid.

    JSON without "dataset" samples instances with bsp -w (see axes_k3n500.json).
    JSON with "dataset" + "labels" loads CNFs with bsp -l (see axes_randsat_n256.json).
    Set "N": null (or omit filtering) to load every size in the dataset; or pass a list.
    """
    run_grid(grid, outdir, jobs=jobs)


def plot(
    data_dir: Path = typer.Argument(
        ...,
        exists=True,
        file_okay=False,
        help="Directory with runs.csv.gz / steps.csv.gz",
    ),
    outdir: Path = typer.Option(
        ...,
        "--outdir",
        "-o",
        help="Directory for PNG figures",
    ),
    alpha: Optional[float] = typer.Option(
        None,
        "--alpha",
        help="Alpha for sigma_vs_depth (default: median alpha in steps)",
    ),
) -> None:
    """Plot BSP experiment csv.gz."""
    make_figures(data_dir, outdir, alpha=alpha)


def label(
    data_dir: Path = typer.Argument(
        ..., exists=True, file_okay=False, help="Grid directory with runs/*/Formula_CNF*.cnf"
    ),
    minisat: str = typer.Option("minisat", "--minisat", help="Minisat binary"),
    timeout_s: int = typer.Option(600, "--timeout", help="Seconds per instance"),
    jobs: int = typer.Option(1, "--jobs", "-j", help="Parallel minisat processes"),
) -> None:
    """Write sat_labels.csv.gz (alpha, seed, SAT|UNSAT|UNKNOWN)."""
    label_grid(data_dir, minisat, timeout_s, jobs)


def grid_app() -> None:
    typer.run(grid)


def plot_app() -> None:
    typer.run(plot)


def label_app() -> None:
    typer.run(label)


if __name__ == "__main__":
    grid_app()
