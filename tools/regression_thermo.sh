#!/bin/sh
# Golden regression for the thermodynamic SP work.
# It rebuilds the solver and the unit tests, operates the golden cases in a
# scratch directory, and compares every output file against tests/golden.
# Numbers may drift in the last bits across builds (fast-math reassociation of
# log and of cancelled sums), so numeric tokens match within 1e-12 absolute and
# 1e-6 relative; all other text must match exactly.
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN="${BIN:-$ROOT/build/release/bsp}"
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

numdiff() {
    awk '
        BEGIN { NUM = "[-+]?([0-9]+(\\.[0-9]*)?|\\.[0-9]+)([eE][-+]?[0-9]+)?" }
        function num_eq(a, b,    d, m) {
            d = a - b; if (d < 0) d = -d
            m = (a < 0) ? -a : a; if (((b < 0) ? -b : b) > m) m = (b < 0) ? -b : b
            return (d <= 1e-12 + 1e-6 * m)
        }
        function line_eq(x, y,    restx, resty, mx, my, lx, ly) {
            restx = x; resty = y
            while (1) {
                mx = match(restx, NUM); lx = RLENGTH
                my = match(resty, NUM); ly = RLENGTH
                if (mx == 0 || my == 0) return (mx == 0 && my == 0 && restx == resty)
                if (substr(restx, 1, mx - 1) != substr(resty, 1, my - 1)) return 0
                if (!num_eq(substr(restx, mx, lx) + 0, substr(resty, my, ly) + 0)) return 0
                restx = substr(restx, mx + lx)
                resty = substr(resty, my + ly)
            }
        }
        FNR == NR { a[FNR] = $0; na = FNR; next }
                { b[FNR] = $0; nb = FNR }
        END {
            if (na != nb) { print "line counts differ"; exit 1 }
            for (i = 1; i <= nb; ++i)
                if (!line_eq(a[i], b[i])) { print "line " i " differs"; exit 1 }
            exit 0
        }
    ' "$1" "$2"
}

cmake --build "$ROOT/build/release" >/dev/null
(cd "$ROOT/build/release" && ctest --output-on-failure)

# Generation mode from the plan (trivial instance), one long generation run that
# exercises decimation and backtracking, and one load mode on a data/ instance.
mkdir -p "$WORK/w30" "$WORK/w100" "$WORK/l80"
(cd "$WORK/w30" && "$BIN" --outdir=. -w 3 3.0 30 --seed=1 --diag=w >stdout.log 2>stderr.log)
(cd "$WORK/w100" && "$BIN" --outdir=. -w 3 4.0 100 --seed=5 --diag-every=25 --diag=p >stdout.log 2>stderr.log)
(cd "$WORK/l80" && "$BIN" --outdir=. -l "$ROOT/data/Formula_CNFK=3N=80alpha=4-SAT_seed=1.cnf" --seed=3 --diag-every=25 --diag=l >stdout.log 2>stderr.log)

status=0
for name in w30 w100 l80; do
    g="$ROOT/tests/golden/$name"
    r="$WORK/$name"
    for gp in "$g"/*; do
        f=${gp##*/}
        # Skip solver-generated manifest: argv and absolute paths differ per machine.
        if [ "$f" = "manifest.json" ]; then
            continue
        fi
        if [ ! -e "$r/$f" ]; then
            echo "regression_thermo: $name missing $f" >&2
            status=1
            continue
        fi
        cmp -s "$gp" "$r/$f" && continue
        if ! numdiff "$gp" "$r/$f" >"$WORK/numdiff.msg"; then
            echo "regression_thermo: $name/$f differs ($(cat "$WORK/numdiff.msg"))" >&2
            diff "$gp" "$r/$f" | head -10 >&2
            status=1
        fi
    done
done
[ "$status" -eq 0 ] && echo "regression_thermo: all golden outputs match"
exit "$status"
