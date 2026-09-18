# Dynamic-I backtracking

`--dynamic-i-backtrack` implements Parisi's assignment-specific release rule
without changing BSP's decimation score or backtracking schedule. The legacy
solver never recomputes surveys for fixed variables, so it releases variables
using `_sC` values saved under obsolete residual formulas. Dynamic-I instead
reconstructs each fixed variable's current incoming cavity warnings from its
unfixed neighbors. If the variable is fixed true, its retained-cluster
fraction is `I(k)=1-sF(k)`; if fixed false, `I(k)=1-sT(k)`. Backtracking
releases the smallest current `I(k)`, i.e. the assignment whose removal
recovers the most clusters. The estimator is read-only, costs
`O(K * degree)` per fixed variable, and requires no extra SP solve or fitted
parameter.

The estimator was checked against 120 full release/reconvergence trials at
K=3, N=300: Pearson correlation 0.99684, all gain signs correct, and MAE
0.00056. Target K=4, N=1000, alpha=9.5 validation over ten seeds improved SAT
from certainty's 5/10 to 6/10, reduced mean absolute complexity change by
8.6%, reduced fixed-depth maximum drop by 39.8%, and reduced fixed-depth
roughness by 4.8%, at 1.94 times the wall time. K=3 target-scale evidence is
recorded separately because finite-size K=3 tests did not show the same gain.
