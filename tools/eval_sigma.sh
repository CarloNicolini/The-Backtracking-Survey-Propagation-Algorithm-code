#!/usr/bin/env bash
# Sigma_res-based eval: run configs over (alpha, seed), collect per-run
# residual complexity + outcome + eta stats from --diag steps CSVs.
# Complements tools/eval_grid.sh (which parses stdout at tiny N).
# Usage (env overrides): K=3 N=500 ALPHAS="4.0 4.2" SEEDS="1 2" \
#   CONFIGS="cert_r0|--scorer=cert --r=0;nn_r09|--scorer=cert --r=0.9 --nn=WEIGHTS" \
#   OUTDIR=/tmp/eval TRIAL_TIMEOUT=600 ./tools/eval_sigma.sh
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MAIN="${MAIN:-$ROOT/build/bsp}"
OUTDIR="${OUTDIR:-/tmp/eval_sigma}"
K="${K:-3}"
N="${N:-500}"
ALPHAS="${ALPHAS:-4.0 4.2}"
SEEDS="${SEEDS:-1 2}"
CONFIGS="${CONFIGS:-cert_r09|--scorer=cert --r=0.9}"
TRIAL_TIMEOUT="${TRIAL_TIMEOUT:-600}"
FLAGS="${FLAGS:-}"
if [[ ! -x "$MAIN" ]]; then echo "missing $MAIN" >&2; exit 1; fi
mkdir -p "$OUTDIR/runs"
SUM="$OUTDIR/summary.tsv"
if [[ ! -f "$SUM" ]]; then
  echo -e "cfg\talpha\tseed\tstatus\tsigma_res\tsigma_res_over_N\tsteps\teta_last\teta_mean\twhite_all_star" > "$SUM"
fi
echo "$CONFIGS" | tr ';' '\n' | while IFS='|' read -r cfg flags; do
  [ -z "$cfg" ] && continue
  for a in $ALPHAS; do for s in $SEEDS; do
    if awk -F'\t' -v c="$cfg" -v a="$a" -v s="$s" \
        '$1==c && $2==a && $3==s {found=1} END {exit !found}' "$SUM" 2>/dev/null; then continue; fi
    tag="${cfg}_a${a}_s${s}"
    rundir="$OUTDIR/runs/$tag"
    mkdir -p "$rundir"
    ( cd "$rundir" && timeout "$TRIAL_TIMEOUT" $MAIN --seed="$s" $FLAGS $flags \
        --diag="$rundir/t" --diag-every=1000000 -w "$K" "$a" "$N" > log.txt 2>&1 )
    rc=$?
    log="$rundir/log.txt"
    status="fail_noconv"
    if grep -q "ASSIGNMENT FOUND" "$log" 2>/dev/null; then status="solve"
    elif grep -q "ASSIGNMENT NOT FOUND" "$log" 2>/dev/null; then status="fail_walksat"
    elif grep -q "Contradiction found" "$log" 2>/dev/null; then status="fail_contra"
    elif grep -q "Negative complexity" "$log" 2>/dev/null; then status="fail_negsig"
    elif grep -q "does not converge" "$log" 2>/dev/null; then status="fail_noconv"
    elif [[ $rc -eq 124 ]]; then status="fail_timeout"
    fi
    stepsf="$rundir/t_steps.csv"
    if [[ -f "$stepsf" ]]; then
      line=$(python3 - "$stepsf" <<'EOF'
import csv,sys
p=sys.argv[1]
rows=[l for l in open(p) if not l.startswith('#')]
r=list(csv.DictReader(rows))
if not r:
    print("0\t0\t0\t0\t0"); sys.exit()
def f(x):
    try: return float(x)
    except Exception: return 0.0
sigs=[f(x['Sigma']) for x in r]
etas=[f(x['eta']) for x in r]
res=0.0
for v in reversed(sigs):
    if v>1e-9:
        res=v; break
n=len(r)
print("%d\t%.10g\t%.6g\t%.6g" % (n,res,n and etas[-1] or 0,sum(etas)/n if n else 0))
EOF
)
      steps=$(echo "$line" | cut -f1); sres=$(echo "$line" | cut -f2)
      etal=$(echo "$line" | cut -f3); etam=$(echo "$line" | cut -f4)
      sresN=$(python3 -c "print($sres/$N)" 2>/dev/null || echo 0)
      if [[ "$status" != "solve" ]]; then sres=0; sresN=0; fi
    else
      steps=0; sres=0; sresN=0; etal=0; etam=0
    fi
    white="NA"
    if [[ "$status" == "solve" ]]; then
      # whitening table follows the "time of whitening" header; data lines
      # have 2 fields (time, white fraction). all-star iff every frac is 1.
      white=$(awk '/time of whitening/{m=1;next} m&&NF==2&&$1~/^[0-9]+$/{if($2==1)c++;else b++} END{if(b>0)print 0; else if(c>0)print 1; else print "NA"}' "$log")
    fi
    echo -e "$cfg\t$a\t$s\t$status\t$sres\t$sresN\t$steps\t$etal\t$etam\t$white" | tee -a "$SUM"
  done; done
done
echo "wrote $SUM"
