"""Regenerate the documentation's Plotly factor graph and measured success curve.

Run with: uv run --group docs python docs/examples/generate_figures.py
The finite-size experiment is a tutorial, not a threshold estimate.
"""
from pathlib import Path
from concurrent.futures import ProcessPoolExecutor
import multiprocessing as mp
import threading
import json

import numpy as np
import pandas as pd
import plotly.graph_objects as go

import pybsp

OUT = Path(__file__).resolve().parents[1] / "_static" / "figures"
N = 64
NUM_INSTANCES = 16
DENSITIES = [3.0, 3.5, 4.0, 4.2, 4.3, 4.5, 4.8]
SEEDS = [1, 2]
TIMEOUT_SECONDS = 1.0


def write_figure(figure, name):
    OUT.mkdir(parents=True, exist_ok=True)
    figure.write_html(OUT / f"{name}.html", include_plotlyjs="../plotly.min.js",
                      config={"displaylogo": False, "responsive": True})


def factor_graph():
    clauses = [[1, -2, 3], [-1, 2, 4], [-2, -3, -4]]
    positions = {f"x{i+1}": (0, 3-i) for i in range(4)}
    positions.update({f"C{i+1}": (2, 2.7-1.2*i) for i in range(3)})
    figure = go.Figure()
    for sign, color, dash, label in [(1, "#4C72B0", "solid", "Positive literal"),
                                    (-1, "#DD8452", "dash", "Negated literal")]:
        x, y = [], []
        for a, clause in enumerate(clauses):
            for lit in clause:
                if np.sign(lit) == sign:
                    xi, yi = positions[f"x{abs(lit)}"]
                    xa, ya = positions[f"C{a+1}"]
                    x.extend([xi, xa, None])
                    y.extend([yi, ya, None])
        figure.add_trace(go.Scatter(x=x, y=y, mode="lines", name=label,
                                   line=dict(color=color, width=2.5, dash=dash), hoverinfo="skip"))
    for names, symbol, color, text in [
        ([f"x{i}" for i in range(1,5)], "circle", "#4C72B0", [f"Variable x{i}" for i in range(1,5)]),
        ([f"C{i}" for i in range(1,4)], "square", "#55A868", ["x1 OR NOT x2 OR x3", "NOT x1 OR x2 OR x4", "NOT x2 OR NOT x3 OR NOT x4"]),
    ]:
        figure.add_trace(go.Scatter(x=[positions[s][0] for s in names], y=[positions[s][1] for s in names],
            mode="markers+text", text=names, textposition="middle center", textfont=dict(color="white",size=14),
            marker=dict(symbol=symbol, size=42, color=color, line=dict(color="white",width=2)),
            customdata=text, hovertemplate="%{customdata}<extra></extra>", showlegend=False))
    figure.update_layout(template="seaborn", title="Variables and clauses of a 3-SAT formula", height=450,
                          margin=dict(l=35,r=35,t=75,b=40), legend=dict(orientation="h",y=1.04,x=0.5,xanchor="center"),
                          font=dict(family="Arial, sans-serif",size=14),
                          xaxis=dict(visible=False,range=[-0.45,2.45]), yaxis=dict(visible=False,range=[-0.4,3.4]))
    return figure


def budgeted_trial(task):
    record, r, seed, generation_seed = task
    formula = record.load()
    token = pybsp.CancellationToken()
    timer = threading.Timer(TIMEOUT_SECONDS, token.cancel)
    timer.start()
    try:
        result = pybsp.solve(formula, pybsp.SolverOptions(r=r, seed=seed), cancel=token)
    finally:
        timer.cancel()
        timer.join()
    verified = result.assignment is not None and formula.is_satisfied(result.assignment)
    return dict(name=record.metadata["name"], N=formula.n, M=len(formula.clauses),
                alpha=len(formula.clauses)/formula.n, configuration=f"r={r:.1f}",
                seed=result.seed, generation_seed=generation_seed, status=result.status,
                reason=result.reason, wall_seconds=result.wall_seconds, verified=verified,
                cooperative_cutoff_seconds=TIMEOUT_SECONDS)


def success_curve():
    tasks = []
    for cohort, alpha in enumerate(DENSITIES):
        dataset = pybsp.Dataset.from_random_ksat(n=N, alpha=alpha, k=3,
                                               num_instances=NUM_INSTANCES, seed=1000+cohort)
        tasks.extend((record, r, seed, 1000+cohort) for record in dataset
                     for r in [0.0, 0.9] for seed in SEEDS)
    rows = []
    with ProcessPoolExecutor(2, mp_context=mp.get_context("spawn")) as pool:
        for row in pool.map(budgeted_trial, tasks, chunksize=1):
            rows.append(row)
            if len(rows) % 64 == 0:
                print("Completed", len(rows), "/", len(tasks), "budgeted attempts", flush=True)
    runs = pd.DataFrame(rows)
    runs.to_csv(OUT / "success_runs.csv",index=False)
    per_instance = runs.groupby(["configuration","N","alpha","name"],as_index=False).agg(solved=("verified","max"))
    summary = per_instance.groupby(["configuration","N","alpha"],as_index=False).agg(
        fraction=("solved","mean"), instances=("solved","size"), solved=("solved","sum"))
    # Wilson intervals treat each formula's best-of-two outcome as one Bernoulli observation.
    z=1.959963984540054
    p, count = summary.fraction, summary.instances
    centre=(p+z*z/(2*count))/(1+z*z/count)
    half=z*np.sqrt(p*(1-p)/count+z*z/(4*count**2))/(1+z*z/count)
    summary["lower"],summary["upper"]=centre-half,centre+half
    summary.to_csv(OUT / "success_summary.csv",index=False)
    figure=go.Figure()
    for i,(name,frame) in enumerate(summary.groupby("configuration",sort=True)):
        figure.add_trace(go.Scatter(x=frame.alpha,y=frame.fraction,mode="lines+markers",name=name,
            line=dict(color=["#4C72B0","#DD8452"][i],width=2.5,dash=["solid","dash"][i]),
            marker=dict(size=8,symbol=["circle","diamond"][i]),
            error_y=dict(type="data",symmetric=False,array=frame.upper-frame.fraction,
                         arrayminus=frame.fraction-frame.lower,thickness=1.2,width=4),
            customdata=frame[["solved","instances"]],
            hovertemplate="α=%{x:.3f}<br>Verified fraction=%{y:.2f}<br>%{customdata[0]} / %{customdata[1]} instances<extra>%{fullData.name}</extra>"))
    figure.update_layout(template="seaborn",height=470,title=dict(text="Verified solve fraction versus clause density<br><sup>N=64 · 16 formulas per density · two seeds · 1 s cooperative cutoff · 95% intervals</sup>"),
        xaxis_title="Clause density α = M / N",yaxis_title="Fraction with a verified solution",
        yaxis=dict(range=[-0.05,1.05],tickformat=".0%"),font=dict(family="Arial, sans-serif",size=14),
        margin=dict(l=65,r=25,t=95,b=60),legend=dict(orientation="h",y=1.05,x=1,xanchor="right"),hovermode="x unified")
    (OUT/"success_experiment.json").write_text(json.dumps(dict(n=N,k=3,instances=NUM_INSTANCES,
        densities=DENSITIES,solver_seeds=SEEDS,generation_seeds=list(range(1000,1000+len(DENSITIES))),
        pybsp_version=pybsp.__version__,cooperative_cutoff_seconds=TIMEOUT_SECONDS,description="finite-size tutorial; unconditional verified solve fraction"),indent=2)+"\n")
    return figure


if __name__ == "__main__":
    OUT.mkdir(parents=True,exist_ok=True)
    write_figure(factor_graph(),"factor_graph")
    write_figure(success_curve(),"success_vs_alpha")
    print("Wrote interactive figures and measured run tables to",OUT)
