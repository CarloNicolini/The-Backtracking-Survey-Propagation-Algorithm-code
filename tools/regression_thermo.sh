#!/bin/sh
# Golden regression for the thermodynamic SP work.
# It rebuilds the solver and the unit tests, operates the golden cases in a
# scratch directory, and diffs every output file against tests/golden/.
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN="$ROOT/build/main"
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

cmake --build "$ROOT/build" >/dev/null
(cd "$ROOT/build" && ctest --output-on-failure)

# Generation mode from the plan (trivial instance), one long generation run that
# exercises decimation and backtracking, and one load mode on a data/ instance.
mkdir -p "$WORK/w30" "$WORK/w100" "$WORK/l80"
(cd "$WORK/w30" && "$BIN" -w 3 3.0 30 --seed=1 --diag=w >stdout.log 2>stderr.log)
(cd "$WORK/w100" && "$BIN" -w 3 4.0 100 --seed=5 --diag-every=25 --diag=p >stdout.log 2>stderr.log)
(cd "$WORK/l80" && "$BIN" -l "$ROOT/data/Formula_CNFK=3N=80alpha=4-SAT_seed=1.cnf" --seed=3 --diag-every=25 --diag=l >stdout.log 2>stderr.log)

status=0
for name in w30 w100 l80; do
    if ! diff -r "$ROOT/tests/golden/$name" "$WORK/$name"; then
        echo "regression_thermo: $name differs from tests/golden/$name" >&2
        status=1
    fi
done
[ "$status" -eq 0 ] && echo "regression_thermo: all golden outputs match"
exit "$status"
