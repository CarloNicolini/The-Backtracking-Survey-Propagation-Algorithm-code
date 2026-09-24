#!/bin/sh
# Smoke test of the soft two-step look-ahead in the limits that must give BSP.
# For each seed it compares the Sigma trajectory (steps.csv without the
# sp_sweeps column) and the final status of the reference binary with:
#   plain       the new binary without softq flags,
#   m1          --softq-m=1, where M <= batch keeps the BSP order,
#   observe     --softq-m=6 --softq-observe, where probes run but the order is kept.
# Usage: REF=/path/master/bsp NEW=/path/new/bsp tools/softq_smoke.sh [N] [alpha] [seeds]
set -eu
REF=${REF:?set REF to the master binary}
NEW=${NEW:?set NEW to the new binary}
N=${1:-300}
ALPHA=${2:-4.1}
SEEDS=${3:-"1 2 3 4 5"}
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

run() { # name binary seed flags...
    name=$1; bin=$2; seed=$3; shift 3
    dir="$WORK/$name/s$seed"
    mkdir -p "$dir"
    (cd "$dir" && "$bin" --seed="$seed" --scorer=cert --r=0.9 "$@" --outdir=. \
        --diag=t --diag-every=1000000 -q -w 3 "$ALPHA" "$N" > log.txt 2>&1 || true)
    cut -d, -f1-10 "$dir/t_steps.csv" > "$dir/traj.csv"
    grep -E "ASSIGNMENT|Contradiction|Negative|converge" "$dir/log.txt" | head -1 > "$dir/status.txt" || true
}

fail=0
for s in $SEEDS; do
    run ref "$REF" "$s"
    run plain "$NEW" "$s"
    run m1 "$NEW" "$s" --softq-m=1
    run observe "$NEW" "$s" --softq-m=6 --softq-observe
    for v in plain m1 observe; do
        if cmp -s "$WORK/ref/s$s/traj.csv" "$WORK/$v/s$s/traj.csv" &&
           cmp -s "$WORK/ref/s$s/status.txt" "$WORK/$v/s$s/status.txt"; then
            echo "seed $s $v: identical ($(wc -l < "$WORK/ref/s$s/traj.csv") steps, $(cat "$WORK/ref/s$s/status.txt"))"
        else
            echo "seed $s $v: DIFFERENT"
            fail=1
        fi
    done
done
exit $fail
