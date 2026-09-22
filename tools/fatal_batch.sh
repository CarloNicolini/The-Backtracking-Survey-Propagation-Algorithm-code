#!/usr/bin/env bash
# Locate the first residual CNF that a SAT solver reports UNSAT, then
# (if a moves CSV is present) list the decimation batch that produced it.
set -euo pipefail
PREFIX="${1:?usage: fatal_batch.sh PREFIX  (files PREFIX_res_s*.cnf)}"
SOLVER=""
for c in cadical kissat minisat; do
  if command -v "$c" >/dev/null 2>&1; then SOLVER="$c"; break; fi
done
if [[ -z "$SOLVER" ]]; then
  echo "No SAT oracle (cadical/kissat/minisat) in PATH."
  echo "Residuals are in ${PREFIX}_res_s*.cnf; install a solver and re-run."
  exit 2
fi
echo "Using SAT oracle: $SOLVER"
shopt -s nullglob
files=( "${PREFIX}"_res_s*.cnf )
if [[ ${#files[@]} -eq 0 ]]; then
  echo "No residuals matching ${PREFIX}_res_s*.cnf"
  exit 1
fi
# numeric sort by step index
IFS=$'\n' files=( $(printf '%s\n' "${files[@]}" | sed 's/.*_res_s\([0-9]*\)\.cnf/\1 &/' | sort -n | awk '{print $2}') )
fatal=""
for f in "${files[@]}"; do
  step="${f##*_res_s}"; step="${step%.cnf}"
  set +e
  out="$("$SOLVER" "$f" 2>&1)"
  rc=$?
  set -e
  # cadical/kissat: 10 SAT, 20 UNSAT; minisat: 10 SAT, 20 UNSAT
  if echo "$out" | grep -qi unsat || [[ $rc -eq 20 ]]; then
    if echo "$out" | grep -qi '^s SATISFIABLE' || [[ $rc -eq 10 ]]; then
      echo "step $step SAT"
      continue
    fi
    echo "FATAL residual at step $step file=$f"
    fatal="$step"
    break
  fi
  echo "step $step SAT (or timeout/unknown rc=$rc)"
done
if [[ -z "$fatal" ]]; then
  echo "No UNSAT residual found (formula stayed SAT through dumped steps)."
  exit 0
fi
moves="${PREFIX}_moves.csv"
if [[ -f "$moves" ]]; then
  # Moves at step s execute after residual s is dumped, producing s+1, so the
  # move that produced the first UNSAT residual ($fatal) ran at $fatal-1.
  echo "Decimation moves that produced it (step $((fatal-1))):"
  awk -F, -v s="$((fatal-1))" 'NR>2 && $1==s && $2=="dec" {print}' "$moves"
fi
