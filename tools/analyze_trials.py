#!/usr/bin/env python3
"""Analyze E4a/E4b lookahead trial logs (PREFIX_trials.csv).

Per step with >=1 converged trial, recompute what each energy rule
(sigma/eta/hybrid) WOULD have picked from the same trial set, and report:
  - agreement: fraction of steps where all three pick the same (var,dir)
  - pairwise agreement sigma-vs-eta, sigma-vs-hybrid, eta-vs-hybrid
  - converged-trial rate, eta_after spread, sigma_after spread
  - trigger-rate curve: fraction of steps with any-trial-min-eta >= T
    (helps calibrate --rollout-eta) -- note this uses TRIAL etas, while the
    E4b trigger uses the parent fixed-point eta; both are reported where
    steps.csv is available via --steps.

Usage:
  python3 tools/analyze_trials.py --trials DIR1/trace_trials.csv ... --out out.tsv
"""

import argparse
import csv
from collections import defaultdict
from pathlib import Path


def pick_sigma(trials):
    conv = [t for t in trials if t["conv"]]
    if not conv:
        return None
    return max(conv, key=lambda t: t["sigma"])


def pick_eta(trials):
    conv = [t for t in trials if t["conv"]]
    if not conv:
        return None
    m = min(t["eta"] for t in conv)
    return max([t for t in conv if t["eta"] == m], key=lambda t: t["sigma"])


def pick_hybrid(trials):
    conv = [t for t in trials if t["conv"]]
    if not conv:
        return None
    m = min(t["eta"] for t in conv)
    gated = [t for t in conv if t["eta"] <= m + 2]
    return max(gated, key=lambda t: t["sigma"])


def key(t):
    return (t["var"], t["dir"]) if t else None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--trials", nargs="+", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()

    rows_out = []
    for path in args.trials:
        steps = defaultdict(list)
        with path.open(newline="") as f:
            lines = (ln for ln in f if not ln.startswith("#"))
            for r in csv.DictReader(lines):
                steps[int(r["step"])].append({
                    "var": r["vertex"], "dir": r["dir"],
                    "conv": r["converged"] == "1",
                    "sigma": float(r["sigma_after"]),
                    "eta": int(r["eta_after"]),
                })
        n = len(steps)
        agree3 = s_e = s_h = e_h = 0
        conv_rates, eta_spreads, sigma_spreads = [], [], []
        min_etas = []
        for trials in steps.values():
            conv = [t for t in trials if t["conv"]]
            conv_rates.append(len(conv) / len(trials))
            if not conv:
                continue
            ps, pe, ph = pick_sigma(trials), pick_eta(trials), pick_hybrid(trials)
            ks, ke, kh = key(ps), key(pe), key(ph)
            if ks == ke == kh:
                agree3 += 1
            if ks == ke:
                s_e += 1
            if ks == kh:
                s_h += 1
            if ke == kh:
                e_h += 1
            etas = [t["eta"] for t in conv]
            sigs = [t["sigma"] for t in conv]
            min_etas.append(min(etas))
            eta_spreads.append(max(etas) - min(etas))
            if len(sigs) > 1:
                sigma_spreads.append(max(sigs) - min(sigs))
        nconv = sum(1 for t in steps.values() if any(x["conv"] for x in t))
        row = {
            "log": path.parent.name,
            "steps": n,
            "steps_with_conv": nconv,
            "mean_conv_rate": sum(conv_rates) / n if n else 0,
            "agree3": agree3 / nconv if nconv else 0,
            "agree_sigma_eta": s_e / nconv if nconv else 0,
            "agree_sigma_hybrid": s_h / nconv if nconv else 0,
            "agree_eta_hybrid": e_h / nconv if nconv else 0,
            "mean_eta_spread": sum(eta_spreads) / len(eta_spreads) if eta_spreads else 0,
            "mean_sigma_spread": sum(sigma_spreads) / len(sigma_spreads) if sigma_spreads else 0,
        }
        for T in (5, 8, 15, 30):
            row[f"frac_mineta_ge_{T}"] = (
                sum(1 for e in min_etas if e >= T) / nconv if nconv else 0)
        rows_out.append(row)
        print(f"{row['log']}: steps={n} conv_rate={row['mean_conv_rate']:.3f} "
              f"agree3={row['agree3']:.3f} s-e={row['agree_sigma_eta']:.3f} "
              f"s-h={row['agree_sigma_hybrid']:.3f} e-h={row['agree_eta_hybrid']:.3f}")

    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows_out[0].keys()),
                           delimiter="\t", lineterminator="\n")
        w.writeheader()
        w.writerows(rows_out)


if __name__ == "__main__":
    main()
