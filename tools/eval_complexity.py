#!/usr/bin/env python3
"""Run BSP policies and summarize their complexity trajectories."""

import argparse
import csv
import html
import shlex
import subprocess
import time
from collections import defaultdict
from pathlib import Path


DEFAULT_CONFIGS = [
    "certainty=--scorer=cert --r=0.9",
    "polarization=--scorer=pol --r=0.9",
    "lookahead4=--scorer=cert --r=0.9 --lookahead-k=4",
]


def parse_config(spec):
    if "=" not in spec:
        raise ValueError(f"config must be NAME=FLAGS, got {spec!r}")
    name, flags = spec.split("=", 1)
    if not name:
        raise ValueError(f"config name is empty in {spec!r}")
    return name, shlex.split(flags)


def classify(log, timed_out):
    if timed_out:
        return "timeout"
    if "ASSIGNMENT FOUND" in log:
        return "sat"
    if "ASSIGNMENT NOT FOUND" in log:
        return "walksat-fail"
    if "Contradiction found" in log:
        return "contradiction"
    if "Negative complexity" in log:
        return "negative-sigma"
    if "does not converge" in log:
        return "sp-nonconvergence"
    return "error"


def read_curve(path):
    with path.open(newline="") as stream:
        lines = (line for line in stream if not line.startswith("#"))
        return list(csv.DictReader(lines))


def summarize_curve(rows):
    sigma = [float(row["Sigma"]) for row in rows]
    deltas = [before - after for before, after in zip(sigma, sigma[1:])]
    transitions = len(deltas)
    has_terminal_zero = bool(sigma) and sigma[-1] == 0.0
    shape_deltas = deltas[:-1] if has_terminal_zero else deltas
    auc = sum((before + after) / 2 for before, after in zip(sigma, sigma[1:]))
    frontier = {}
    for row in rows:
        fixed = int(row["n_fixed"])
        value = float(row["Sigma"])
        frontier[fixed] = max(value, frontier.get(fixed, value))
    fixed_points = sorted(frontier.items())
    fixed_slopes = [
        (before[1] - after[1]) / (after[0] - before[0])
        for before, after in zip(fixed_points, fixed_points[1:])
    ]
    fixed_span = (
        fixed_points[-1][0] - fixed_points[0][0] if len(fixed_points) > 1 else 0
    )
    fixed_auc = sum(
        (after[0] - before[0]) * (before[1] + after[1]) / 2
        for before, after in zip(fixed_points, fixed_points[1:])
    )
    return {
        "steps": transitions,
        "sigma_initial": sigma[0] if sigma else 0.0,
        "sigma_final": sigma[-1] if sigma else 0.0,
        "max_drop": max(deltas, default=0.0),
        "terminal_drop": deltas[-1] if has_terminal_zero and deltas else 0.0,
        "max_drop_preterminal": max(shape_deltas, default=0.0),
        "mean_drop": (
            (sigma[0] - sigma[-1]) / transitions if transitions else 0.0
        ),
        "mean_abs_delta": (
            sum(abs(delta) for delta in deltas) / transitions
            if transitions
            else 0.0
        ),
        "mean_abs_delta_preterminal": (
            sum(abs(delta) for delta in shape_deltas) / len(shape_deltas)
            if shape_deltas
            else 0.0
        ),
        "auc_sigma": auc,
        "auc_per_step": auc / transitions if transitions else 0.0,
        "max_fixed": fixed_points[-1][0] if fixed_points else 0,
        "fixed_frontier_max_drop": max(fixed_slopes, default=0.0),
        "fixed_frontier_mean_drop": (
            (fixed_points[0][1] - fixed_points[-1][1]) / fixed_span
            if fixed_span
            else 0.0
        ),
        "fixed_frontier_roughness": (
            sum(
                abs(slope) * (after[0] - before[0])
                for slope, before, after in zip(
                    fixed_slopes, fixed_points, fixed_points[1:]
                )
            )
            / fixed_span
            if fixed_span
            else 0.0
        ),
        "fixed_frontier_auc": fixed_auc,
        "fixed_frontier_auc_per_level": (
            fixed_auc / fixed_span if fixed_span else 0.0
        ),
        "eta_mean": (
            sum(float(row["eta"]) for row in rows) / len(rows) if rows else 0.0
        ),
    }


def run_one(args, config, flags, seed):
    run_dir = args.out / f"{config}_seed{seed}"
    run_dir.mkdir(parents=True, exist_ok=True)
    prefix = run_dir / "trace"
    log_path = run_dir / "run.log"
    command = [
        str(args.main),
        f"--seed={seed}",
        f"--diag={prefix}",
        "--diag-every=1000000000",
        *flags,
        "-w",
        str(args.k),
        str(args.alpha),
        str(args.n),
    ]
    started = time.perf_counter()
    timed_out = False
    try:
        result = subprocess.run(
            command,
            cwd=run_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=args.timeout,
            check=False,
        )
        output = result.stdout
        returncode = result.returncode
    except subprocess.TimeoutExpired as error:
        timed_out = True
        output = error.stdout or ""
        if isinstance(output, bytes):
            output = output.decode(errors="replace")
        returncode = 124
    elapsed = time.perf_counter() - started
    log_path.write_text(output)

    curve_path = run_dir / "trace_steps.csv"
    rows = read_curve(curve_path) if curve_path.exists() else []
    metrics = summarize_curve(rows)
    metrics.update(
        {
            "config": config,
            "seed": seed,
            "status": classify(output, timed_out),
            "returncode": returncode,
            "wall_seconds": elapsed,
            "curve": curve_path,
        }
    )
    return metrics


def write_summary(path, records):
    fields = [
        "config",
        "seed",
        "status",
        "returncode",
        "steps",
        "sigma_initial",
        "sigma_final",
        "max_drop",
        "terminal_drop",
        "max_drop_preterminal",
        "mean_drop",
        "mean_abs_delta",
        "mean_abs_delta_preterminal",
        "auc_sigma",
        "auc_per_step",
        "max_fixed",
        "fixed_frontier_max_drop",
        "fixed_frontier_mean_drop",
        "fixed_frontier_roughness",
        "fixed_frontier_auc",
        "fixed_frontier_auc_per_level",
        "eta_mean",
        "wall_seconds",
    ]
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(
            stream, fieldnames=fields, delimiter="\t", lineterminator="\n"
        )
        writer.writeheader()
        for record in records:
            writer.writerow({field: record[field] for field in fields})


def mean(records, field):
    return sum(float(record[field]) for record in records) / len(records)


def write_aggregate(path, records):
    groups = defaultdict(list)
    for record in records:
        groups[record["config"]].append(record)
    fields = [
        "config",
        "runs",
        "sat",
        "sp_nonconvergence",
        "mean_max_drop",
        "mean_max_drop_preterminal",
        "mean_drop",
        "mean_abs_delta",
        "mean_abs_delta_preterminal",
        "mean_auc_per_step",
        "mean_fixed_frontier_max_drop",
        "mean_fixed_frontier_roughness",
        "mean_fixed_frontier_auc_per_level",
        "mean_steps",
        "mean_wall_seconds",
    ]
    aggregate = []
    for config, group in groups.items():
        aggregate.append(
            {
                "config": config,
                "runs": len(group),
                "sat": sum(record["status"] == "sat" for record in group),
                "sp_nonconvergence": sum(
                    record["status"] == "sp-nonconvergence" for record in group
                ),
                "mean_max_drop": mean(group, "max_drop"),
                "mean_max_drop_preterminal": mean(
                    group, "max_drop_preterminal"
                ),
                "mean_drop": mean(group, "mean_drop"),
                "mean_abs_delta": mean(group, "mean_abs_delta"),
                "mean_abs_delta_preterminal": mean(
                    group, "mean_abs_delta_preterminal"
                ),
                "mean_auc_per_step": mean(group, "auc_per_step"),
                "mean_fixed_frontier_max_drop": mean(
                    group, "fixed_frontier_max_drop"
                ),
                "mean_fixed_frontier_roughness": mean(
                    group, "fixed_frontier_roughness"
                ),
                "mean_fixed_frontier_auc_per_level": mean(
                    group, "fixed_frontier_auc_per_level"
                ),
                "mean_steps": mean(group, "steps"),
                "mean_wall_seconds": mean(group, "wall_seconds"),
            }
        )
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(
            stream, fieldnames=fields, delimiter="\t", lineterminator="\n"
        )
        writer.writeheader()
        writer.writerows(aggregate)
    return aggregate


def write_svg(path, records, n, x_field="step", x_label="SP transition"):
    curves = []
    for record in records:
        rows = read_curve(record["curve"]) if record["curve"].exists() else []
        points = [
            (int(row[x_field]), float(row["Sigma"]) / n)
            for row in rows
        ]
        if points:
            curves.append((record["config"], record["seed"], points))
    if not curves:
        return

    width, height = 1000, 650
    left, right, top, bottom = 90, 30, 40, 75
    plot_width = width - left - right
    plot_height = height - top - bottom
    max_x = max(point[0] for _, _, curve in curves for point in curve) or 1
    values = [point[1] for _, _, curve in curves for point in curve]
    min_y, max_y = min(values), max(values)
    if max_y == min_y:
        max_y += 1.0
    pad = 0.05 * (max_y - min_y)
    min_y -= pad
    max_y += pad

    names = list(dict.fromkeys(config for config, _, _ in curves))
    palette = ["#0072B2", "#D55E00", "#009E73", "#CC79A7", "#E69F00"]
    colors = {name: palette[index % len(palette)] for index, name in enumerate(names)}

    def sx(value):
        return left + value / max_x * plot_width

    def sy(value):
        return top + (max_y - value) / (max_y - min_y) * plot_height

    body = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<line x1="{left}" y1="{top}" x2="{left}" y2="{top + plot_height}" '
        'stroke="#222"/>',
        f'<line x1="{left}" y1="{top + plot_height}" '
        f'x2="{left + plot_width}" y2="{top + plot_height}" stroke="#222"/>',
    ]
    for config, seed, curve in curves:
        points = " ".join(f"{sx(x):.2f},{sy(y):.2f}" for x, y in curve)
        body.append(
            f'<polyline points="{points}" fill="none" stroke="{colors[config]}" '
            'stroke-width="1.5" opacity="0.55"/>'
        )
    body.extend(
        [
            f'<text x="{left + plot_width / 2}" y="{height - 22}" '
            'text-anchor="middle" font-family="sans-serif" font-size="16">'
            f"{html.escape(x_label)}</text>",
            f'<text x="22" y="{top + plot_height / 2}" text-anchor="middle" '
            'transform="rotate(-90 22 '
            f'{top + plot_height / 2})" font-family="sans-serif" font-size="16">'
            "Sigma / N</text>",
            f'<text x="{left}" y="{top + plot_height + 25}" '
            'text-anchor="middle" font-family="sans-serif" font-size="13">0</text>',
            f'<text x="{left + plot_width}" y="{top + plot_height + 25}" '
            f'text-anchor="middle" font-family="sans-serif" font-size="13">{max_x}</text>',
            f'<text x="{left - 8}" y="{sy(min_y) + 5}" text-anchor="end" '
            f'font-family="sans-serif" font-size="13">{min_y:.3g}</text>',
            f'<text x="{left - 8}" y="{sy(max_y) + 5}" text-anchor="end" '
            f'font-family="sans-serif" font-size="13">{max_y:.3g}</text>',
        ]
    )
    legend_x = left + 15
    for index, name in enumerate(names):
        y = top + 20 + 22 * index
        body.append(
            f'<line x1="{legend_x}" y1="{y}" x2="{legend_x + 28}" y2="{y}" '
            f'stroke="{colors[name]}" stroke-width="3"/>'
        )
        body.append(
            f'<text x="{legend_x + 36}" y="{y + 5}" font-family="sans-serif" '
            f'font-size="14">{html.escape(name)}</text>'
        )
    body.append("</svg>")
    path.write_text("\n".join(body) + "\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--main", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--k", type=int, required=True)
    parser.add_argument("--n", type=int, required=True)
    parser.add_argument("--alpha", type=float, required=True)
    parser.add_argument("--seeds", default="1,2,3")
    parser.add_argument("--timeout", type=float, default=600)
    parser.add_argument(
        "--config",
        action="append",
        help="NAME=FLAGS; repeat for multiple policies",
    )
    args = parser.parse_args()
    args.main = args.main.resolve()
    args.out = args.out.resolve()
    args.out.mkdir(parents=True, exist_ok=True)

    configs = [parse_config(spec) for spec in (args.config or DEFAULT_CONFIGS)]
    seeds = [int(seed) for seed in args.seeds.split(",")]
    records = []
    for config, flags in configs:
        for seed in seeds:
            print(f"running {config} seed={seed}", flush=True)
            record = run_one(args, config, flags, seed)
            records.append(record)
            print(
                f"  {record['status']} steps={record['steps']} "
                f"max_drop={record['max_drop']:.6g} "
                f"wall={record['wall_seconds']:.2f}s",
                flush=True,
            )

    write_summary(args.out / "summary.tsv", records)
    aggregate = write_aggregate(args.out / "aggregate.tsv", records)
    write_svg(args.out / "sigma_curves.svg", records, args.n)
    write_svg(
        args.out / "sigma_vs_fixed.svg",
        records,
        args.n,
        x_field="n_fixed",
        x_label="fixed variables",
    )
    for row in aggregate:
        print(
            f"{row['config']}: sat={row['sat']}/{row['runs']} "
            f"max_drop={row['mean_max_drop']:.6g} "
            f"mean_abs_delta={row['mean_abs_delta']:.6g} "
            f"wall={row['mean_wall_seconds']:.2f}s"
        )


if __name__ == "__main__":
    main()
