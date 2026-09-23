"""Seaborn figures from experiment csv.gz, styled with SciencePlots."""

from __future__ import annotations

from contextlib import contextmanager
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd
import scienceplots  # noqa: F401  — registers styles with matplotlib
import seaborn as sns

from pybsp.parse import load_csv_gz

# science + nature as in SciencePlots docs; no-latex so PNG works without TeX
_STYLE = ("science", "nature", "no-latex")
FIGSIZE = (5, 2)


@contextmanager
def science_style():
    with plt.style.context(_STYLE):
        plt.rcParams["font.family"] = "sans-serif"
        plt.rcParams["font.sans-serif"] = [
            "Helvetica",
            "Helvetica Neue",
            "Arial",
            "DejaVu Sans",
        ]
        yield


def binomial_se(p: float, n: int) -> float:
    if n <= 0:
        return float("nan")
    return (p * (1.0 - p) / n) ** 0.5


def success_table(runs: pd.DataFrame) -> pd.DataFrame:
    g = runs.groupby(["cfg", "alpha"], as_index=False).agg(
        n=("status", "size"),
        n_solve=("status", lambda s: int((s == "solve").sum())),
    )
    g["p_solve"] = g["n_solve"] / g["n"]
    g["se"] = [binomial_se(p, n) for p, n in zip(g["p_solve"], g["n"])]
    return g


def plot_success_vs_alpha(runs: pd.DataFrame, out: Path) -> None:
    tab = success_table(runs)
    with science_style():
        fig, ax = plt.subplots(figsize=FIGSIZE)
        sns.lineplot(
            data=tab,
            x="alpha",
            y="p_solve",
            hue="cfg",
            marker="o",
            ax=ax,
        )
        for _, sub in tab.groupby("cfg"):
            ax.errorbar(
                sub["alpha"],
                sub["p_solve"],
                yerr=sub["se"],
                fmt="none",
                capsize=3,
                alpha=0.7,
            )
        ax.set_ylabel("P(solve)")
        ax.set_xlabel(r"$\alpha$")
        ax.set_ylim(-0.05, 1.05)
        ax.set_title("Success rate vs alpha")
        fig.tight_layout()
        out.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(out, dpi=300)
        plt.close(fig)


def plot_sigma_mean_vs_step(
    steps: pd.DataFrame,
    out: Path,
    *,
    alpha: float | None = None,
) -> None:
    df = steps
    if alpha is not None:
        df = df.loc[df["alpha"] == alpha]
    if df.empty:
        raise ValueError("no steps rows to plot (check --alpha filter)")
    agg = (
        df.groupby(["cfg", "step"], as_index=False)["Sigma_over_N"]
        .mean()
        .rename(columns={"Sigma_over_N": "mean_Sigma_over_N"})
    )
    with science_style():
        fig, ax = plt.subplots(figsize=FIGSIZE)
        sns.lineplot(
            data=agg,
            x="step",
            y="mean_Sigma_over_N",
            hue="cfg",
            ax=ax,
        )
        title = r"Mean $\Sigma/N$ vs step"
        if alpha is not None:
            title += f" ($\\alpha$={alpha})"
        ax.set_title(title)
        ax.set_xlabel("step")
        ax.set_ylabel(r"mean $\Sigma/N$")
        fig.tight_layout()
        out.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(out, dpi=300)
        plt.close(fig)


def make_figures(
    data_dir: Path,
    fig_dir: Path,
    *,
    alpha: float | None = None,
) -> None:
    runs = load_csv_gz(data_dir / "runs.csv.gz")
    steps = load_csv_gz(data_dir / "steps.csv.gz")
    if runs.empty:
        raise FileNotFoundError(f"no runs.csv.gz in {data_dir}")
    plot_success_vs_alpha(runs, fig_dir / "success_vs_alpha.png")
    print(f"wrote {fig_dir / 'success_vs_alpha.png'}")
    if not steps.empty:
        if alpha is None:
            alphas = sorted(steps["alpha"].unique())
            alpha = float(alphas[len(alphas) // 2])
        plot_sigma_mean_vs_step(steps, fig_dir / "sigma_mean_vs_step.png", alpha=alpha)
        print(f"wrote {fig_dir / 'sigma_mean_vs_step.png'} (alpha={alpha})")
