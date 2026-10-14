import numpy as np
import pytest
pytest.importorskip("sklearn")
from sklearn.base import clone

from pybsp import CancellationToken, Formula, KSATSolver, iter_solve, solve_batch
from test_benchmark import make_dataset
from pybsp import Dataset


def test_solver_clone_outputs_and_repeated_fit():
    solver = KSATSolver(r=0.8, random_state=7, show_progress=False)
    assert clone(solver).get_params() == solver.get_params()
    formulas = [Formula([[1]], n=3), Formula([[1], [-1]], n=3)]
    assignments = solver.fit_predict(formulas)
    assert formulas[0].is_satisfied(assignments[0]) and assignments[1] is None
    assert solver.statuses_.tolist() == ['solved', 'failed']
    assert solver.report_.metrics()['solve_rate'] == 0.5
    solver.set_params(r=0.9)
    assert len(solver.fit_predict(formulas[:1])) == 1
    assert len(solver.run_table_) == 1
    solver.set_params(r=1)
    with pytest.raises(ValueError):
        solver.fit_predict(formulas)
    assert not hasattr(solver, 'assignments_')


def test_array_shapes_and_generators():
    matrix = np.array([[1, -2], [2, 3]], dtype=np.int32)
    assert len(solve_batch(matrix, show_progress=False).assignments) == 1
    report = solve_batch(np.stack([matrix, matrix]), seeds=[1, 2], show_progress=False)
    assert len(report.assignments) == 4
    assert report.runs.input_index.tolist() == [0, 0, 1, 1]
    assert report.runs.trial.tolist() == [0, 1, 0, 1]
    assert len(list(iter_solve((Formula([[1]], n=2) for _ in range(3)), batch_size=1,
                               show_progress=False))) == 3
    with pytest.raises(ValueError):
        solve_batch(np.array([1]), show_progress=False)


def test_lazy_dataset_frames_and_parallel_equivalence(tmp_path):
    root, _ = make_dataset(tmp_path, [
        (f'N3_M1_id{i}.cnf', 'p cnf 3 1\n1 0\n', 1, '1 2 3 0') for i in range(1, 5)
    ])
    dataset = Dataset.from_randsatbench(root)
    assert len(dataset) == 4 and dataset[0].load().n == 3
    assert len(dataset[:2]) == 2
    assert [len(b) for b in dataset.iter_batches(3)] == [3, 1]
    frame = dataset.to_frame()
    assert 'reference_assignment' not in frame
    frame.loc[0, 'N'] = 999
    assert dataset.instances.iloc[0].N == 3
    a = solve_batch(dataset, seeds=[1, 2], show_progress=False, batch_size=1)
    b = solve_batch(dataset, seeds=[1, 2], show_progress=False, n_jobs=2, batch_size=2)
    assert a.runs.drop(columns='wall_seconds').equals(b.runs.drop(columns='wall_seconds'))
    for x, y in zip(a.assignments, b.assignments):
        assert np.array_equal(x, y) and not y.flags.writeable
    assert b.summary(by=()).iloc[0].sat_recovery == 1


def test_cancellation_and_invalid_jobs():
    token = CancellationToken()
    token.cancel()
    report = solve_batch([Formula([[1]], n=3)], cancel=token, show_progress=False)
    assert report.cancelled and report.runs.empty
    with pytest.raises(ValueError):
        solve_batch([], n_jobs=0, show_progress=False)


def test_parallel_cancellation_and_owned_retained_arrays():
    token = CancellationToken()
    stream = iter_solve((Formula([[1]], n=3) for _ in range(100)), n_jobs=2,
                        batch_size=1, cancel=token, show_progress=False, keep_results=True)
    first = next(stream)
    token.cancel()
    remaining = list(stream)
    assert len(remaining) < 10  # Pending chunks finish; the rest are never dispatched.
    assert first[2].formula.is_satisfied(first[1])
    assert not first[2].solutions.flags.writeable
    assert not first[2].surveys.flags.writeable


def test_failed_paths_do_not_abort_batch_and_no_output_files(tmp_path):
    formula = Formula([[1]], n=3)
    report = solve_batch([tmp_path / 'missing.cnf', formula], show_progress=False)
    assert report.runs.status.tolist() == ['error', 'solved']
    assert report.assignments[0] is None and formula.is_satisfied(report.assignments[1])
    assert list(tmp_path.iterdir()) == []
    frame = report.to_frame()
    frame.loc[1, 'status'] = 'changed'
    assert report.runs.iloc[1].status == 'solved'
