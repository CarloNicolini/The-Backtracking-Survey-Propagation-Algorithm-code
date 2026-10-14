"""Native API contracts; set BSP_TEST_EXECUTABLE for CLI parity checks."""
import gc
import json
import logging
import os
from pathlib import Path
import signal
import subprocess
import threading
import time
from concurrent.futures import ThreadPoolExecutor

import numpy as np
import pandas as pd
import pytest

from pybsp import CancellationToken, Formula, SolverError, SolverOptions, solve


def test_inputs_and_dimacs(tmp_path):
    formula = Formula(np.array([[1, -2], [-1, 3]], dtype=np.int32), n=4)
    assert formula.n == 4
    assert Formula.from_dimacs("c comment\np cnf 4 2\n1\nc inside clause\n-2 0 -1 3 0\n") == formula
    path = tmp_path / "input.cnf"
    path.write_text(formula.to_dimacs())
    assert Formula.from_file(path) == formula
    assert Formula([[1], [-1, 2], [3, 4, -5]], n=6).n == 6


@pytest.mark.parametrize("text", ["1 0", "p cnf 1 1\n1", "p cnf 1 2\n1 0", "p cnf 1 1\n2 0", "p cnf 1 0\np cnf 1 0"])
def test_invalid_dimacs(text):
    with pytest.raises(ValueError):
        Formula.from_dimacs(text)


@pytest.mark.parametrize("clauses,n", [([[0]], 1), ([[2]], 1), ([[1, -1]], 1), ([[1.5]], 1), ([[True]], 1), ([], 0)])
def test_invalid_formula(clauses, n):
    with pytest.raises((ValueError, TypeError)):
        Formula(clauses, n=n)


@pytest.mark.parametrize("kwargs", [
    {"r": 1}, {"r": np.nan}, {"eps": 0}, {"eps": np.inf}, {"seed": 0}, {"theta": -1},
    {"damping": 1}, {"cav_temp": -1}, {"act_temp": 1}, {"scorer": "fth"},
    {"fe_backtrack": True}, {"tsallis_kappa": 1}, {"rsb2_x": 0.5, "cav_temp": 0.2},
    {"rsb2_stab": True, "rsb_m": 1}, {"rsb2_x": 0.5, "sp2_mu2": 1},
    {"oracle_pick": 1}, {"rsb2_pop": 1}, {"sp2_x": 0}, {"diag_every": 0},
    {"scorer": "gamma:1garbage"}, {"scorer": "soft_cert:0"}, {"log_level": "invalid"},
    {"veto": 1}, {"lookahead": 1.5},
])
def test_invalid_options_raise_without_poisoning_run(kwargs):
    formula = Formula([[1]], n=1)
    with pytest.raises((ValueError, TypeError)):
        solve(formula, SolverOptions(**kwargs))
    assert solve(formula, SolverOptions(seed=1)).status == "solved"


@pytest.mark.parametrize("clauses,n", [([[1]], 1), ([[1], [-2]], 3), ([], 3)])
def test_verified_assignments(clauses, n):
    formula = Formula(clauses, n=n)
    result = solve(formula, SolverOptions(seed=1))
    assert result.status == "solved"
    assert result.assignment.shape == (n,)
    assert all(formula.is_satisfied(row) for row in result.solutions)


@pytest.mark.parametrize("clauses", [[[1], [-1]], [[]]])
def test_failure_is_not_unsat(clauses):
    result = solve(Formula(clauses, n=1), SolverOptions(seed=1))
    assert result.status == "failed"
    assert result.reason
    assert result.assignment is None
    assert result.solutions.shape == (0, 1)


def test_no_files_capture_and_owned_arrays(tmp_path, monkeypatch, capfd):
    monkeypatch.chdir(tmp_path)
    formula = Formula.random(k=3, alpha=3, n=32, seed=1)
    result = solve(formula, SolverOptions(variable_history=True, residual_history=True, diag_every=2))
    assert Path.cwd() == tmp_path
    assert list(tmp_path.iterdir()) == []
    captured = capfd.readouterr()
    assert captured.out == captured.err == ""
    assert "walksat version" in result.stdout
    assert not result.stderr
    assert result.logs
    assert result.variables.step.mod(2).eq(0).all()
    assert all(step % 2 == 0 for step in result.residuals)
    assert result.surveys.shape == (32, 3)
    array = result.assignment
    expected = array.copy()
    del result
    gc.collect()
    np.testing.assert_array_equal(array, expected)
    assert formula.is_satisfied(array)


def test_defaults_and_explicit_export(tmp_path):
    formula = Formula.random(k=3, alpha=3, n=32, seed=1)
    result = solve(formula)
    assert result.variables.empty and not result.populations and not result.residuals
    destination = result.export(tmp_path / "export")
    assert Formula.from_file(destination / "formula.cnf").clauses == formula.clauses
    np.testing.assert_array_equal(np.load(destination / "solutions.npy"), result.solutions)
    pd.testing.assert_frame_equal(pd.read_csv(destination / "t_steps.csv"), result.steps, check_exact=False)
    assert json.loads((destination / "manifest.json").read_text())["seed"] == 1
    with pytest.raises(FileExistsError):
        result.export(destination)


def test_logger_and_stderr(caplog):
    with caplog.at_level(logging.INFO):
        result = solve(Formula([[1], [-1]], n=1), SolverOptions(seed=1), logger=logging.getLogger("bsp.test"))
    assert "Contradiction" in result.stderr
    assert any(record.channel == "stderr" and record.level == "ERROR" for record in result.logs)
    assert "Contradiction" in caplog.text


@pytest.mark.parametrize("options", [
    SolverOptions(cav_temp=0.2, rsb2_stab=True, variable_history=True),
    SolverOptions(rsb2_x=0.5, rsb2_pop=8, population_history=True),
    SolverOptions(sp2_mu2=2, sp2_pop=8, population_history=True),
    SolverOptions(scorer="rsb", rsb_m=0.1),
])
def test_repeated_runs_reset_all_state(options):
    formula = Formula.random(k=3, alpha=3, n=32, seed=1)
    a = solve(formula, options)
    solve(formula, SolverOptions(scorer="pol", cav_temp=0.4, tsallis_kappa=2))
    b = solve(formula, options)
    assert a.status == b.status
    np.testing.assert_array_equal(a.solutions, b.solutions)
    for name in a.tables:
        pd.testing.assert_frame_equal(a.tables[name], b.tables[name])
    for step, matrix in a.populations.items():
        np.testing.assert_array_equal(matrix, b.populations[step])


def test_parallel_calls_are_serialized():
    formula = Formula.random(k=3, alpha=3, n=32, seed=1)
    with ThreadPoolExecutor(max_workers=2) as pool:
        results = list(pool.map(lambda _: solve(formula), range(4)))
    for result in results[1:]:
        np.testing.assert_array_equal(results[0].solutions, result.solutions)
        pd.testing.assert_frame_equal(results[0].steps, result.steps)


def test_cancellation_and_recovery():
    token = CancellationToken()
    token.cancel()
    assert solve(Formula([[1]], n=1), cancel=token).status == "cancelled"
    formula = Formula.random(k=3, alpha=4.2, n=5000, seed=1)
    token = CancellationToken()
    timer = threading.Timer(0.03, token.cancel)
    timer.start()
    start = time.monotonic()
    try:
        result = solve(formula, cancel=token)
    finally:
        timer.join()
    assert result.status == "cancelled"
    assert time.monotonic() - start < 5
    assert solve(Formula([[1]], n=1)).status == "solved"


def test_keyboard_interrupt_is_cooperative():
    formula = Formula.random(k=3, alpha=4.2, n=5000, seed=1)
    timer = threading.Timer(0.03, lambda: os.kill(os.getpid(), signal.SIGINT))
    timer.start()
    try:
        result = solve(formula)
    finally:
        timer.join()
    assert result.status == "cancelled"


@pytest.mark.parametrize("extra,options", [
    ([], SolverOptions(variable_history=True)),
    (["--cav-temp=0.2", "--act-temp=0.1", "--fe-backtrack", "--dynamic-i-backtrack"], SolverOptions(cav_temp=0.2, act_temp=0.1, fe_backtrack=True, dynamic_i_backtrack=True, variable_history=True)),
    (["--rsb2-stab"], SolverOptions(rsb2_stab=True, variable_history=True)),
    (["--rsb2-x=0.5", "--rsb2-pop=8"], SolverOptions(rsb2_x=0.5, rsb2_pop=8, variable_history=True)),
    (["--sp2-mu2=2", "--sp2-pop=8"], SolverOptions(sp2_mu2=2, sp2_pop=8, variable_history=True)),
    (["--cav-temp=0.2", "--tsallis-kappa=2"], SolverOptions(cav_temp=0.2, tsallis_kappa=2, variable_history=True)),
    (["--lookahead=2", "--veto", "--adaptive-r", "--scorer=soft_cert:2"], SolverOptions(lookahead=2, veto=True, adaptive_r=True, scorer="soft_cert:2", variable_history=True)),
])
@pytest.mark.parametrize("loaded", [False, True])
def test_cli_parity(tmp_path, extra, options, loaded):
    executable = os.environ.get("BSP_TEST_EXECUTABLE")
    if not executable:
        pytest.skip("set BSP_TEST_EXECUTABLE to enable CLI comparisons")
    formula = Formula.random(k=3, alpha=3, n=32, seed=1)
    args = ["-w", "3", "3", "32"]
    if loaded:
        path = tmp_path / "input.cnf"
        path.write_text(formula.to_dimacs())
        formula = Formula.from_file(path)
        args = ["-l", str(path)]
        from dataclasses import replace
        options = replace(options, seed=1)
    result = solve(formula, options)
    run = subprocess.run([executable, "--seed=1", "--diag=t", f"--outdir={tmp_path / 'cli'}", *extra, *args], capture_output=True, text=True, timeout=30)
    assert run.returncode == 0, run.stderr
    assert result.status == "solved", result.reason
    for name, filename in [("steps", "t_steps.csv"), ("variables", "t_vars.csv"), ("moves", "t_moves.csv"), ("rsb2", "t_rsb2.csv"), ("sp2", "t_sp2.csv")]:
        path = tmp_path / "cli" / filename
        if path.exists():
            expected = pd.read_csv(path, comment="#")
            actual = result.tables[name]
            if name == "moves":
                actual = actual.loc[actual.step >= 0].reset_index(drop=True)
            pd.testing.assert_frame_equal(actual, expected, check_dtype=False, check_exact=False, rtol=1e-8, atol=1e-9)
    solutions = sorted((tmp_path / "cli").glob("Solution_tot*.txt"), key=lambda p: int(p.name.split("_")[3]))
    if solutions:
        expected = np.array([np.loadtxt(path, dtype=np.int32) for path in solutions])
        np.testing.assert_array_equal(result.solutions, expected)


def test_minisat_tempfiles_are_cleaned(tmp_path, monkeypatch):
    monkeypatch.setenv("TMPDIR", str(tmp_path))
    record = tmp_path / "inputs.txt"
    executable = tmp_path / "minisat"
    executable.write_text(f'#!/bin/sh\nprintf "%s\\n" "$1" >> "{record}"\nexit 10\n')
    executable.chmod(0o755)
    result = solve(Formula.random(k=3, alpha=3, n=32, seed=1), SolverOptions(oracle_dir=True, minisat=str(executable)))
    assert result.status == "solved"
    paths = record.read_text().splitlines()
    assert paths and all(not Path(path).exists() for path in paths)
    assert not list(tmp_path.glob("pybsp-oracle-*"))


def test_minisat_error_has_partial_result(tmp_path, monkeypatch):
    monkeypatch.setenv("TMPDIR", str(tmp_path))
    formula = Formula.random(k=3, alpha=3, n=32, seed=1)
    with pytest.raises(SolverError) as error:
        solve(formula, SolverOptions(oracle_dir=True, minisat=str(tmp_path / "missing")))
    assert error.value.result.status == "error"
    assert "minisat not found" in error.value.result.stderr
    assert not error.value.result.steps.empty
    assert not list(tmp_path.glob("pybsp-oracle-*"))
    assert solve(formula).status == "solved"


def test_minisat_cancellation_cleans_input(tmp_path, monkeypatch):
    monkeypatch.setenv("TMPDIR", str(tmp_path))
    executable = tmp_path / "minisat"
    executable.write_text('#!/bin/sh\nexec sleep 10\n')
    executable.chmod(0o755)
    token = CancellationToken()
    timer = threading.Timer(0.1, token.cancel)
    timer.start()
    try:
        result = solve(Formula.random(k=3, alpha=3, n=32, seed=1), SolverOptions(oracle_dir=True, minisat=str(executable)), cancel=token)
    finally:
        timer.join()
    assert result.status == "cancelled"
    assert not list(tmp_path.glob("pybsp-oracle-*"))


def test_population_edge_provenance_and_lifetime():
    formula = Formula.random(k=3, alpha=3, n=32, seed=1)
    result = solve(formula, SolverOptions(sp2_mu2=2, sp2_pop=8, population_history=True))
    assert "sp2: mu2=" in result.stdout
    matrix = result.populations[0]
    edges = result.tables["population_edges"]
    assert matrix.shape == (len(edges), 8)
    for row in edges.itertuples():
        assert row.vertex * row.sign in formula.clauses[row.clause - 1]
    expected = matrix.copy()
    del result
    gc.collect()
    np.testing.assert_array_equal(matrix, expected)


def test_cancellation_while_waiting_for_run():
    formula = Formula.random(k=3, alpha=4.2, n=5000, seed=1)
    first, second = CancellationToken(), CancellationToken()
    with ThreadPoolExecutor(max_workers=2) as pool:
        a = pool.submit(solve, formula, cancel=first)
        time.sleep(0.03)
        b = pool.submit(solve, formula, cancel=second)
        second.cancel()
        assert b.result(timeout=5).status == "cancelled"
        first.cancel()
        assert a.result(timeout=5).status == "cancelled"
