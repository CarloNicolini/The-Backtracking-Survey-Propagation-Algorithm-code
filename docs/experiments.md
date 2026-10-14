# Experiments across clause density

A solver's performance depends on the number of variables, the clause density
and the organization of the solution space. A useful first measurement is the
fraction of independent formulas for which the solver returns a verified
satisfying assignment. For $B$ formulas at a given density, define

$$
\widehat p_{\mathrm{solve}}(\alpha)
=\frac{1}{B}\sum_{b=1}^{B}
\mathbf{1}\{\text{a returned assignment for formula }b\text{ has }E=0\}.
$$

Every formula belongs in this denominator, including those on which search
fails. The resulting curve combines the prevalence of satisfiable formulas
with the solver's ability to find their solutions. If reference SAT labels are
available, recovery among verified SAT instances answers the conditional question
of how often known satisfiable formulas are solved.

## A measured example

The figure below was generated with the native solver on random 3-SAT formulas
with $N=64$. Each density uses 16 independent formulas, shared by the two
configurations. An instance counts as solved if either of two fixed solver
seeds finds a verified assignment. Each attempt has a one-second cooperative
cutoff implemented with a cancellation token. Error bars are 95% Wilson
intervals across formulas. These small samples illustrate the measurement and its uncertainty.
They do not locate an asymptotic algorithmic or satisfiability threshold.
Cancellation is checked at safe boundaries, so the cutoff is approximate.
Cancelled attempts count as unsuccessful unless they already returned a
verified assignment. The run table retains these outcomes explicitly.

```{raw} html
<iframe src="_static/figures/success_vs_alpha.html" title="Measured verified solve fraction versus clause density" width="100%" height="500" style="border:0" loading="lazy"></iframe>
```

The [summary table](_static/figures/success_summary.csv),
[individual runs](_static/figures/success_runs.csv) and
[experiment parameters](_static/figures/success_experiment.json) accompany the
figure. The horizontal coordinate is the actual density $M/N$, with
$M=\lceil\alpha N\rceil$ when an ensemble is generated from a requested density.

## Create the chart in a notebook

The {download}`experiment notebook <../notebooks/native_bsp_experiments.ipynb>`
runs a small generated ensemble by default. It also supports RandSATBench through
`DATA_SOURCE` and `DATASET_ROOT`. Start it in the project environment:

```bash
uv sync --locked --group notebooks
uv run --group notebooks jupyter lab
```

After collecting `validation_runs`, group repeated solver attempts by formula
before averaging their outcomes:

```python
import plotly.express as px

per_formula = validation_runs.groupby(
    ["configuration", "N", "alpha", "name"], as_index=False,
).agg(solved=("verified", "max"))
success = per_formula.groupby(
    ["configuration", "N", "alpha"], as_index=False,
).agg(fraction=("solved", "mean"), instances=("solved", "size"))

fig = px.line(
    success, x="alpha", y="fraction", color="configuration", facet_col="N",
    markers=True, template="seaborn",
    labels={"alpha": "Clause density α = M / N", "fraction": "Verified solve fraction"},
)
fig.update_yaxes(range=[-0.05, 1.05], tickformat=".0%")
fig.update_layout(title="Best-of-seeds solve fraction", hovermode="x unified")
fig.show()
```

Plotly's [templates](https://plotly.com/python/templates/) provide a consistent
starting style. The `seaborn` template gives these examples their restrained
palette and grid. Hovering reveals individual measurements, and legend entries
can be toggled to compare curves. Documentation figures use a locally bundled
Plotly runtime. The chart remains interactive without contacting a plotting
service.

The notebook also includes matplotlib/seaborn recovery plots, runtime ECDFs and
paired comparisons. It saves experiment files only when `SAVE_RESULTS=True`.

## Comparing configurations

Use the same formulas for every configuration and keep generation, selection
and trajectory seeds separate. Select parameters on validation formulas, then
measure the chosen configuration on held-out formulas. The notebook implements
this split explicitly. A fixed number of seeds gives a comparable best-of-seeds
success criterion.

`KSATSolver.evaluate()` returns measurements that can be reused for several
metrics. `metrics()` reports solve rate, SAT recovery, native wall time and
execution errors. `summary()` groups instance-level recovery by size and density.
A reference assignment can differ from the returned assignment while both
satisfy every clause. Hamming agreement describes their relation, with
correctness established by clause verification.

Uncertainty should reflect independent formulas. Repeated trajectories on the
same formula share its difficulty. For paired comparisons, form the difference
between configurations on each formula before estimating uncertainty. Runtime
plots should retain unsuccessful attempts. Native wall time excludes input
loading, reference checks and process startup.

Save the experiment configuration alongside its run table and manifests.
Manifests contain options, actual seeds and software/platform information.
Record the source revision and build settings separately when reproducing a
research result. The current API has no general effort budget, so timing
comparisons alone do not establish performance at equal computational effort.

## Relation to the PRE paper

The finite-formula experiments here use the C++ `bsp_core` solver. The PRE paper
under `papers/survey-propagation-2rsb/paper_pre/` obtains its figures from sparse
ensemble population dynamics in `t0/` and dense Gaussian calculations in
`numerics/`. Those settings describe distributions of messages or stationary
order parameters, each with its own numerical validation procedure.

Independent replication of the paper requires population-size scans, several
seeds and initializations, late-time histories and thermodynamic-estimator
checks. Some archived thermodynamic values remain provisional. The C++ `sp2`
options provide finite-formula diagnostics. Their measurements can complement
this study without supplying an additional decimation policy.

The documentation figures can be regenerated explicitly with:

```bash
uv run --group docs python docs/examples/generate_figures.py
uv run --group docs sphinx-build -W -b html docs docs/_build/html
```

The first command runs the finite-size experiment and writes its figure assets
and tables. A routine documentation build uses those stored measurements.
