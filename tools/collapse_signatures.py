#!/usr/bin/env python3
"""E1: do step-level signatures predict eventual BSP failure?

Reads eval_complexity.py result dirs (summary.tsv + */trace_steps.csv).
For each run, truncates the Sigma trace at fractions f of its length and
computes prefix features; then reports AUC of (feature -> final failure)
at each truncation. A feature with high AUC at small f is an early warning
that a soft controller could act on.

Steps-only features (no vars.csv needed):
  drop_max      max single-step Sigma drop in prefix
  rough_mean    mean |dSigma| in prefix
  eta_max       max SP iterations in prefix
  eta_last      eta at truncation point
  sigma_frac    Sigma(trunc)/Sigma(0)
  back_frac     fraction of back moves in prefix
  slope         (Sigma(0)-Sigma(trunc))/steps (mean descent rate)

Usage:
  python3 tools/collapse_signatures.py --dirs results/soft-e0-k3-n300-a4.0 \
      results/soft-e0-k3-n300-a4.15 --out results/soft-e1-collapse.tsv
"""

import argparse
import csv
import math
from pathlib import Path


def read_steps(path):
    with path.open(newline="") as stream:
        lines = (ln for ln in stream if not ln.startswith("#"))
        return list(csv.DictReader(lines))


def read_summary(path):
    out = {}
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            out[(row["config"], row["seed"])] = row["status"]
    return out


def auc(pos, neg):
    """Mann-Whitney AUC of scores: pos=failures, neg=successes."""
    if not pos or not neg:
        return float("nan")
    wins = 0.0
    for p in pos:
        for n in neg:
            if p > n:
                wins += 1.0
            elif p == n:
                wins += 0.5
    return wins / (len(pos) * len(neg))


def prefix_features(rows, end):
    seg = rows[: end + 1]
    sigmas = [float(r["Sigma"]) for r in seg]
    deltas = [a - b for a, b in zip(sigmas, sigmas[1:])]
    etas = [float(r["eta"]) for r in seg]
    backs = sum(1 for r in seg if r["move"] == "back")
    steps = max(len(seg) - 1, 1)
    return {
        "drop_max": max(deltas, default=0.0),
        "rough_mean": sum(abs(d) for d in deltas) / steps if deltas else 0.0,
        "eta_max": max(etas, default=0.0),
        "eta_last": etas[-1] if etas else 0.0,
        "sigma_frac": (sigmas[-1] / sigmas[0]) if sigmas and sigmas[0] else 0.0,
        "back_frac": backs / len(seg) if seg else 0.0,
        "slope": ((sigmas[0] - sigmas[-1]) / steps) if sigmas else 0.0,
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dirs", nargs="+", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--fracs", default="0.1,0.25,0.5,0.75")
    args = ap.parse_args()

    fracs = [float(x) for x in args.fracs.split(",")]
    feat_names = ["drop_max", "rough_mean", "eta_max", "eta_last",
                  "sigma_frac", "back_frac", "slope"]
    # sigma_frac predicts failure when LOW -> flip sign for AUC
    flip = {"sigma_frac"}

    runs = []  # (dir, config, seed, failed, rows)
    for d in args.dirs:
        status = read_summary(d / "summary.tsv")
        for (config, seed), st in status.items():
            p = d / f"{config}_seed{seed}" / "trace_steps.csv"
            if not p.exists():
                continue
            rows = read_steps(p)
            if len(rows) < 5:
                continue
            runs.append((d.name, config, seed, st != "sat", rows))

    n_fail = sum(1 for r in runs if r[3])
    print(f"runs={len(runs)} failures={n_fail} successes={len(runs)-n_fail}")

    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["dir", "frac", "feature", "auc",
                                          "n_fail", "n_sat"],
                           delimiter="\t", lineterminator="\n")
        w.writeheader()
        for d in args.dirs:
            subset = [r for r in runs if r[0] == d.name]
            for fr in fracs:
                feats_fail = {k: [] for k in feat_names}
                feats_sat = {k: [] for k in feat_names}
                for _, _, _, failed, rows in subset:
                    end = max(1, min(len(rows) - 1, int(fr * (len(rows) - 1))))
                    feats = prefix_features(rows, end)
                    tgt = feats_fail if failed else feats_sat
                    for k in feat_names:
                        tgt[k].append(feats[k])
                for k in feat_names:
                    pf, ps = feats_fail[k], feats_sat[k]
                    if k in flip:
                        pf = [-v for v in pf]
                        ps = [-v for v in ps]
                    a = auc(pf, ps)
                    w.writerow({"dir": d.name, "frac": fr, "feature": k,
                                "auc": f"{a:.4f}" if not math.isnan(a) else "nan",
                                "n_fail": len(pf), "n_sat": len(ps)})
                    print(f"{d.name} f={fr:<5} {k:<11} AUC={a:.3f} "
                          f"(fail={len(pf)}, sat={len(ps)})")


if __name__ == "__main__":
    main()
