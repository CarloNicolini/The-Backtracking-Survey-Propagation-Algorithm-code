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
        cmp_left="$gp"
        cmp_right="$r/$f"
        if [ "$f" = "stdout.log" ]; then
            grep -v '^Output directory:' "$gp" > "$WORK/gold.stdout"
            grep -v '^Output directory:' "$r/$f" > "$WORK/run.stdout"
            cmp_left="$WORK/gold.stdout"
            cmp_right="$WORK/run.stdout"
        fi
        cmp -s "$cmp_left" "$cmp_right" && continue
        if ! numdiff "$cmp_left" "$cmp_right" >"$WORK/numdiff.msg"; then
            echo "regression_thermo: $name/$f differs ($(cat "$WORK/numdiff.msg"))" >&2
            diff "$cmp_left" "$cmp_right" | head -10 >&2 || true
            status=1
        fi
    done
done
[ "$status" -eq 0 ] && echo "regression_thermo: all golden outputs match"

# --rsb-gamma=0 must reproduce the hard baseline. --rsb-gamma=0.5 must move
# the complexity trace. A strong bias can make SP quit with a contradiction
# (exit 255); that is a controlled solver stop, not a crash.
mkdir -p "$WORK/gbase" "$WORK/g0" "$WORK/g05"
(cd "$WORK/gbase" && "$BIN" --outdir=. -w 3 3.0 20 --seed=7 >stdout.log 2>stderr.log)
(cd "$WORK/g0" && "$BIN" --outdir=. --rsb-gamma=0 -w 3 3.0 20 --seed=7 >stdout.log 2>stderr.log)
g05_rc=0
(cd "$WORK/g05" && "$BIN" --outdir=. --rsb-gamma=0.5 -w 3 3.0 20 --seed=7 >stdout.log 2>stderr.log) || g05_rc=$?
if [ "$g05_rc" -ne 0 ] && [ "$g05_rc" -ne 255 ]; then
    echo "regression_thermo: --rsb-gamma=0.5 exited $g05_rc" >&2
    status=1
fi
trace_sigma() {
    awk 'NF==5 && $1 ~ /^[0-9]/ {print}' "$1"
}
trace_sigma "$WORK/gbase/stdout.log" > "$WORK/gbase.trace"
trace_sigma "$WORK/g0/stdout.log" > "$WORK/g0.trace"
trace_sigma "$WORK/g05/stdout.log" > "$WORK/g05.trace"
if ! numdiff "$WORK/gbase.trace" "$WORK/g0.trace" >"$WORK/numdiff.msg"; then
    echo "regression_thermo: --rsb-gamma=0 differs from baseline ($(cat "$WORK/numdiff.msg"))" >&2
    status=1
fi
if cmp -s "$WORK/gbase.trace" "$WORK/g05.trace"; then
    echo "regression_thermo: --rsb-gamma=0.5 matches baseline" >&2
    status=1
fi

# --rsb-m=M is the MMW closure gamma = M ln2, so both flags give one trace.
mkdir -p "$WORK/m03" "$WORK/gm03"
m03_rc=0
gm03_rc=0
(cd "$WORK/m03" && "$BIN" --outdir=. --rsb-m=0.3 -w 3 3.0 20 --seed=7 >stdout.log 2>stderr.log) || m03_rc=$?
(cd "$WORK/gm03" && "$BIN" --outdir=. --rsb-gamma=0.20794415416798359 -w 3 3.0 20 --seed=7 >stdout.log 2>stderr.log) || gm03_rc=$?
trace_sigma "$WORK/m03/stdout.log" > "$WORK/m03.trace"
trace_sigma "$WORK/gm03/stdout.log" > "$WORK/gm03.trace"
if [ "$m03_rc" -ne "$gm03_rc" ] || ! numdiff "$WORK/m03.trace" "$WORK/gm03.trace" >"$WORK/numdiff.msg"; then
    echo "regression_thermo: --rsb-m=0.3 differs from --rsb-gamma=0.3 ln2" >&2
    status=1
fi

# At T -> 0 the Bethe complexity Sigma_e = G + y e must equal the legacy Sigma.
mkdir -p "$WORK/t0" "$WORK/tlow"
(cd "$WORK/t0" && "$BIN" --outdir=. -w 3 4.0 100 --seed=5 >stdout.log 2>stderr.log)
(cd "$WORK/tlow" && "$BIN" --outdir=. --cav-temp=1e-9 -w 3 4.0 100 --seed=5 >stdout.log 2>stderr.log)
trace_sigma "$WORK/t0/stdout.log" > "$WORK/t0.trace"
trace_sigma "$WORK/tlow/stdout.log" > "$WORK/tlow.trace"
if ! numdiff "$WORK/t0.trace" "$WORK/tlow.trace" >"$WORK/numdiff.msg"; then
    echo "regression_thermo: Sigma_e at --cav-temp=1e-9 differs from T=0 ($(cat "$WORK/numdiff.msg"))" >&2
    status=1
fi

# A vanishing Tsallis kappa runs the outer loop but keeps y_eff = y, so the
# trace must equal plain SP-y at the same cavity temperature.
mkdir -p "$WORK/spy" "$WORK/k0"
(cd "$WORK/spy" && "$BIN" --outdir=. --cav-temp=0.5 -w 3 4.0 100 --seed=5 >stdout.log 2>stderr.log)
(cd "$WORK/k0" && "$BIN" --outdir=. --cav-temp=0.5 --tsallis-kappa=1e-12 -w 3 4.0 100 --seed=5 >stdout.log 2>stderr.log)
trace_sigma "$WORK/spy/stdout.log" > "$WORK/spy.trace"
trace_sigma "$WORK/k0/stdout.log" > "$WORK/k0.trace"
if ! numdiff "$WORK/spy.trace" "$WORK/k0.trace" >"$WORK/numdiff.msg"; then
    echo "regression_thermo: --tsallis-kappa=1e-12 differs from SP-y ($(cat "$WORK/numdiff.msg"))" >&2
    status=1
fi

exit "$status"
