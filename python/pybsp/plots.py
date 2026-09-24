"""Seaborn figures from experiment csv.gz, styled with SciencePlots."""

from __future__ import annotations

from contextlib import contextmanager
from pathlib import Path
from sys import platform

import matplotlib.pyplot as plt
import pandas as pd
import scienceplots  # noqa: F401  — registers styles with matplotlib
import seaborn as sns

from pybsp.parse import load_csv_gz
from pybsp.stats import label_agreement_table, paired_vs_baseline, success_table

# science + nature as in SciencePlots docs; no-latex so PNG works without TeX
_STYLE = ("science", "nature", "no-latex")
FIGSIZE = (5, 2)
DEPTH_BINS = 50
# Helvetica on macOS; TeX Gyre Heros (Helvetica clone) on Linux
_SANS = ["TeX Gyre Heros", "Arial", "Liberation Sans", "DejaVu Sans"] if platform.startswith(
    "linux"
) else ["Helvetica", "Helvetica Neue", "Arial", "DejaVu Sans"]


@contextmanager
def science_style():
    with plt.style.context(_STYLE):
        plt.rcParams["font.family"] = "sans-serif"
        plt.rcParams["font.sans-serif"] = _SANS
        yield


def save(fig, out: Path) -> None:
    fig.tight_layout()
    out.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out, dpi=300)
    plt.close(fig)
    print(f"wrote {out}")


def _n_values(df: pd.DataFrame) -> list[int]:
    if "N" not in df.columns:
        return []
    return sorted(int(x) for x in df["N"].unique())


def _line_with_se(ax, tab: pd.DataFrame, *, y: str, title: str, ylabel: str) -> None:
    sns.lineplot(data=tab, x="alpha", y=y, hue="cfg", marker="o", ax=ax)
    if "se" in tab.columns:
        for _, sub in tab.groupby("cfg"):
            ax.errorbar(
                sub["alpha"], sub[y], yerr=sub["se"], fmt="none", capsize=3, alpha=0.7
            )
    ax.set_ylabel(ylabel)
    ax.set_xlabel(r"$\alpha$")
    ax.set_ylim(-0.05, 1.05)
    ax.set_title(title)


def plot_success_vs_alpha(runs: pd.DataFrame, out: Path, title: str = "Success rate vs alpha") -> None:
    tab = success_table(runs)
    ns = _n_values(tab)
    with science_style():
        if len(ns) <= 1:
            fig, ax = plt.subplots(figsize=FIGSIZE)
            _line_with_se(ax, tab, y="p_solve", title=title, ylabel="P(solve)")
            save(fig, out)
            return
        fig, axes = plt.subplots(
            1, len(ns), figsize=(2.4 * len(ns), 2.2), sharey=True, squeeze=False
        )
        for ax, n in zip(axes.flat, ns):
            _line_with_se(
                ax,
                tab.loc[tab["N"] == n],
                y="p_solve",
                title=f"{title} (N={n})",
                ylabel="P(solve)",
            )
            if ax is not axes.flat[0]:
                ax.set_ylabel("")
        save(fig, out)


def plot_sigma_vs_depth(
    steps: pd.DataFrame, runs: pd.DataFrame, out: Path, *, alpha: float, N: int | None = None
) -> None:
    """Median Sigma/N against the fixed fraction 1 - N_t/N, with the interquartile band."""
    df = steps.loc[steps["alpha"] == alpha]
    if N is not None and "N" in df.columns:
        df = df.loc[df["N"] == N]
    if df.empty:
        raise ValueError("no steps rows to plot (check --alpha / N filter)")
    on = [c for c in ("cfg", "N", "alpha", "seed") if c in df.columns and c in runs.columns]
    df = df.merge(runs[on + ["status"]], on=on)
    df = df.assign(
        depth=((1.0 - df["Nt"] / df["N"]) * DEPTH_BINS).round() / DEPTH_BINS,
        outcome=(df["status"] == "solve").map({True: "solved", False: "failed"}),
    )
    per_run = df.groupby(["cfg", "outcome", "seed", "depth"], as_index=False)[
        "Sigma_over_N"
    ].mean()
    tag = f"$\\alpha$={alpha}" + (f", N={N}" if N is not None else "")
    with science_style():
        fig, ax = plt.subplots(figsize=FIGSIZE)
        sns.lineplot(
            data=per_run,
            x="depth",
            y="Sigma_over_N",
            hue="cfg",
            style="outcome",
            estimator="median",
            errorbar=("pi", 50),
            ax=ax,
        )
        sns.move_legend(ax, "center left", bbox_to_anchor=(1, 0.5))
        ax.set_title(f"Median $\\Sigma/N$ vs fixed fraction ({tag})")
        ax.set_xlabel(r"fixed fraction $1-N_t/N$")
        ax.set_ylabel(r"$\Sigma/N$")
        save(fig, out)


def plot_status_by_alpha(runs: pd.DataFrame, out: Path) -> None:
    """Fraction of each exit status per alpha, one panel per config (and N if many)."""
    ns = _n_values(runs)
    if len(ns) > 1:
        for n in ns:
            plot_status_by_alpha(runs.loc[runs["N"] == n], out.with_name(f"{out.stem}_N{n}{out.suffix}"))
        return
    keys = ["cfg", "alpha"]
    frac = runs.groupby(keys)["status"].value_counts(normalize=True).unstack(fill_value=0.0)
    cfgs = frac.index.get_level_values("cfg").unique()
    ncols = min(4, len(cfgs))
    nrows = -(-len(cfgs) // ncols)
    with science_style():
        fig, axes = plt.subplots(
            nrows, ncols, figsize=(2.2 * ncols, 1.8 * nrows), sharey=True, squeeze=False
        )
        for ax, cfg in zip(axes.flat, cfgs):
            frac.loc[cfg].plot(kind="bar", stacked=True, ax=ax, legend=False, width=0.85)
            ax.set_title(cfg)
            ax.set_xlabel(r"$\alpha$")
        for ax in axes.flat[len(cfgs) :]:
            ax.set_visible(False)
        axes.flat[0].set_ylabel("fraction")
        handles, labels = axes.flat[0].get_legend_handles_labels()
        fig.legend(handles, labels, loc="lower center", ncol=len(labels), bbox_to_anchor=(0.5, 1.0))
        save(fig, out)


def plot_agreement_vs_alpha(runs: pd.DataFrame, labels: pd.DataFrame, out: Path) -> None:
    """P(solve) split by ground-truth SAT / UNSAT from sat_labels.csv.gz."""
    tab = label_agreement_table(runs, labels)
    if tab.empty:
        return
    tab.to_csv(out.with_suffix(".csv"), index=False)
    print(f"wrote {out.with_suffix('.csv')}")
    ns = _n_values(tab)
    with science_style():
        if len(ns) <= 1:
            fig, ax = plt.subplots(figsize=FIGSIZE)
            sns.lineplot(
                data=tab, x="alpha", y="p_solve", hue="cfg", style="sat", marker="o", ax=ax
            )
            sns.move_legend(ax, "center left", bbox_to_anchor=(1, 0.5))
            ax.set_ylabel("P(solve)")
            ax.set_xlabel(r"$\alpha$")
            ax.set_ylim(-0.05, 1.05)
            ax.set_title("P(solve) by ground-truth label")
            save(fig, out)
            return
        g = sns.relplot(
            data=tab,
            x="alpha",
            y="p_solve",
            hue="cfg",
            style="sat",
            col="N",
            kind="line",
            marker="o",
            height=2.2,
            aspect=1.1,
        )
        g.set_axis_labels(r"$\alpha$", "P(solve)")
        g.set(ylim=(-0.05, 1.05))
        g.fig.suptitle("P(solve) by ground-truth label", y=1.05)
        out.parent.mkdir(parents=True, exist_ok=True)
        g.savefig(out, dpi=300)
        plt.close(g.fig)
        print(f"wrote {out}")


def plot_paired_vs_baseline(runs: pd.DataFrame, out: Path, baseline: str = "cert") -> None:
    """Paired success difference to the baseline on the same seeds."""
    tab = paired_vs_baseline(runs, baseline)
    if tab.empty:
        return
    tab.to_csv(out.with_suffix(".csv"), index=False)
    print(f"wrote {out.with_suffix('.csv')}")
    ns = _n_values(tab)
    with science_style():
        if len(ns) <= 1:
            fig, ax = plt.subplots(figsize=FIGSIZE)
            cfgs = sorted(tab["cfg"].unique())
            palette = dict(zip(cfgs, sns.color_palette("tab10", n_colors=len(cfgs))))
            sns.lineplot(data=tab, x="alpha", y="diff", hue="cfg", palette=palette, ax=ax)
            sig = tab[tab["p_mcnemar"] < 0.05]
            if not sig.empty:
                sns.scatterplot(
                    data=sig,
                    x="alpha",
                    y="diff",
                    hue="cfg",
                    palette=palette,
                    ax=ax,
                    legend=False,
                    s=12,
                )
            sns.move_legend(ax, "center left", bbox_to_anchor=(1, 0.5))
            ax.axhline(0.0, color="grey", lw=0.5)
            ax.set_xlabel(r"$\alpha$")
            ax.set_ylabel(f"P(solve) - P(solve | {baseline})")
            ax.set_title(f"Paired difference to {baseline} (dots: McNemar p<0.05)")
            save(fig, out)
            return
        g = sns.relplot(
            data=tab,
            x="alpha",
            y="diff",
            hue="cfg",
            col="N",
            kind="line",
            height=2.2,
            aspect=1.1,
        )
        for ax in g.axes.flat:
            ax.axhline(0.0, color="grey", lw=0.5)
        g.set_axis_labels(r"$\alpha$", f"P(solve) - P(solve | {baseline})")
        g.fig.suptitle(f"Paired difference to {baseline}", y=1.05)
        out.parent.mkdir(parents=True, exist_ok=True)
        g.savefig(out, dpi=300)
        plt.close(g.fig)
        print(f"wrote {out}")


def make_figures(data_dir: Path, fig_dir: Path, *, alpha: float | None = None) -> None:
    runs = load_csv_gz(data_dir / "runs.csv.gz")
    steps = load_csv_gz(data_dir / "steps.csv.gz")
    if runs.empty:
        raise FileNotFoundError(f"no runs.csv.gz in {data_dir}")
    plot_success_vs_alpha(runs, fig_dir / "success_vs_alpha.png")
    labels = load_csv_gz(data_dir / "sat_labels.csv.gz")
    if not labels.empty:
        on = [c for c in ("N", "alpha", "seed") if c in runs.columns and c in labels.columns]
        sat_runs = runs.merge(labels.loc[labels["sat"] == "SAT", on])
        if not sat_runs.empty:
            plot_success_vs_alpha(
                sat_runs, fig_dir / "success_vs_alpha_sat.png", "P(solve | SAT)"
            )
        plot_agreement_vs_alpha(runs, labels, fig_dir / "agreement_vs_alpha.png")
    plot_status_by_alpha(runs, fig_dir / "status_vs_alpha.png")
    if (runs["cfg"] == "cert").any():
        plot_paired_vs_baseline(runs, fig_dir / "paired_vs_cert.png")
    if not steps.empty:
        ns = _n_values(steps) or [None]
        if alpha is None:
            alphas = sorted(steps["alpha"].unique())
            alpha = float(alphas[len(alphas) // 2])
        for n in ns:
            suffix = f"_N{n}" if n is not None and len(ns) > 1 else ""
            # alphas are snapped to M/N per size, so match the nearest one of this N
            sub = steps if n is None else steps.loc[steps["N"] == n]
            alpha_n = float(min(sub["alpha"].unique(), key=lambda a: abs(a - alpha)))
            plot_sigma_vs_depth(
                steps,
                runs,
                fig_dir / f"sigma_vs_depth{suffix}.png",
                alpha=alpha_n,
                N=n,
            )
