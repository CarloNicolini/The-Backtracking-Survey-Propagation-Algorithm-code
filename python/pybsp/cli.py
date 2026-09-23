"""Typer CLI: bsp-grid and bsp-plot."""

from __future__ import annotations

from pathlib import Path
from typing import Optional

import typer

from pybsp.grid import run_grid
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
    """Run a BSP experiment grid."""
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
        help="Alpha for sigma_mean_vs_step (default: median alpha in steps)",
    ),
) -> None:
    """Plot BSP experiment csv.gz."""
    make_figures(data_dir, outdir, alpha=alpha)


def grid_app() -> None:
    typer.run(grid)


def plot_app() -> None:
    typer.run(plot)


if __name__ == "__main__":
    grid_app()
