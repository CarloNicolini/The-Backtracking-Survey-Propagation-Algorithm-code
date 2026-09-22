#!/usr/bin/env bash
# Small ablation grid: scorer x r, recording solve/fail and last Sigma/N from stdout.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MAIN="${MAIN:-$ROOT/build/bsp}"
OUT="${OUT:-$ROOT/tools/eval_grid.tsv}"
if [[ ! -x "$MAIN" ]]; then
  echo "missing $MAIN (build the solver first)" >&2
  exit 1
fi
K="${K:-3}"
N="${N:-50}"
ALPHAS="${ALPHAS:-3.0 3.5 4.0}"
SCORERS="${SCORERS:-cert pol gamma:1.0}"
RS="${RS:-0 0.9}"
SEEDS="${SEEDS:-1 2}"
FLAGS="${FLAGS:-}"
echo -e "scorer\tr\talpha\tseed\tstatus\tlast_sigma_over_N" > "$OUT"
for sc in $SCORERS; do
  for r in $RS; do
    for a in $ALPHAS; do
      for s in $SEEDS; do
        set +e
        log="$("$MAIN" $FLAGS --scorer="$sc" --r="$r" --seed="$s" -w "$K" "$a" "$N" 2>&1)"
        rc=$?
        set -e
        status="fail"
        echo "$log" | grep -q "ASSIGNMENT FOUND" && status="solve"
        last="$(echo "$log" | awk 'NF==5 && $3 ~ /^-?[0-9]/ {v=$4} END{print v}')"
        echo -e "$sc\t$r\t$a\t$s\t$status\t${last:-NA}" | tee -a "$OUT"
      done
    done
  done
done
echo "wrote $OUT"
