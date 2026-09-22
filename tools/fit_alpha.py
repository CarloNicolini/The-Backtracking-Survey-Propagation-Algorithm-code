#!/usr/bin/env python3
"""Linear alpha_a fit from eval_sigma.sh summary.tsv (stdlib only).

For each cfg: mean Sigma_res/N per alpha, least-squares line, x-intercept.
Usage: fit_alpha.py summary.tsv
"""
import csv
import sys
from collections import defaultdict

def main(path):
    per = defaultdict(lambda: defaultdict(list))
    with open(path) as f:
        for r in csv.DictReader(f, delimiter="\t"):
            try:
                per[r["cfg"]][float(r["alpha"])].append(float(r["sigma_res_over_N"]))
            except ValueError:
                pass
    for cfg in sorted(per):
        xs, ys = [], []
        for a in sorted(per[cfg]):
            v = per[cfg][a]
            xs.append(a)
            ys.append(sum(v) / len(v))
        n = len(xs)
        if n < 2:
            print(f"{cfg}: need >=2 alphas, have {n}")
            continue
        mx = sum(xs) / n
        my = sum(ys) / n
        den = sum((x - mx) ** 2 for x in xs)
        if den == 0:
            print(f"{cfg}: degenerate alphas")
            continue
        slope = sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / den
        icept = my - slope * mx
        alpha_a = -icept / slope if slope != 0 else float("nan")
        solves = " ".join(f"{a}:{sum(1 for _ in per[cfg][a])}" for a in sorted(per[cfg]))
        print(f"{cfg}: slope={slope:.6g} intercept={icept:.6g} "
              f"alpha_a~{alpha_a:.6g} n_per_alpha=[{solves}]")
        for a in sorted(per[cfg]):
            v = per[cfg][a]
            print(f"    alpha={a} mean={sum(v)/len(v):.6g} vals={[f'{x:.4g}' for x in v]}")

if __name__ == "__main__":
    main(sys.argv[1])
