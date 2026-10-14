"""Labelled native evaluation: references, split isolation, and bounded retention."""
import csv
from pathlib import Path

import numpy as np
import pytest

from pybsp import CancellationToken, Formula, SatDataset, SolverOptions, analyze_dataset


def make_dataset(tmp_path, rows, split="test"):
    root = tmp_path / "datasets" / "3SAT"
    cnfs = root / split
    cnfs.mkdir(parents=True, exist_ok=True)
    labels = tmp_path / "3SAT" / f"{split}_labels.csv"
    labels.parent.mkdir(exist_ok=True)
    with labels.open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["cnf_file", "sat", "assignment"])
        for name, text, status, assignment in rows:
            (cnfs / name).write_text(text)
            writer.writerow([f"{split}-final/{name}", status, assignment])
    return root, labels


def test_reference_comparison_repeats_and_no_files(tmp_path):
    root, labels = make_dataset(tmp_path, [
        ("N3_M1_id1.cnf", "p cnf 3 1\n1 0\n", 1, "-3 1 2 0"),
        ("N3_M2_id2.cnf", "p cnf 3 2\n1 0\n-1 0\n", 0, ""),
        ("N3_M1_id3.cnf", "p cnf 3 1\n2 0\n", -1, ""),
    ])
    dataset = SatDataset.from_randsatbench(root)
    assert dataset.labels_csv == labels
    assert dataset.instances.reference_status.tolist() == ["SAT", "UNKNOWN", "UNSAT"]
    before, cwd = set(tmp_path.rglob("*")), Path.cwd()
    progress = []
    report = analyze_dataset(dataset, seeds=[1, 2], keep_results=True,
                             progress=lambda done, total, row: progress.append((done, total)))
    assert Path.cwd() == cwd and set(tmp_path.rglob("*")) == before
    assert progress == [(i, 6) for i in range(1, 7)]
    assert len(report.runs) == len(report.results) == 6
    sat = report.runs.loc[report.runs.reference_status == "SAT"]
    assert sat.reference_valid.all() and sat.verified.all()
    reference = np.array([1, 2, -3])
    for row in sat.itertuples():
        result = report.results[(row.name, row.trial)]
        assert result.formula.is_satisfied(result.assignment)
        assert row.hamming_distance == np.count_nonzero(result.assignment != reference)
        assert row.agreement == 1 - row.hamming_distance / 3
    summary = report.summary(by=()).iloc[0]
    assert summary.instances == 3 and summary.solved_instances == 2
    assert summary.reference_sat_instances == 1 and summary.sat_recovery == 1
    assert summary.reference_unsat_instances == 1 and summary.failed_runs == 2
    assert not report.runs.label_conflict.any()


def test_split_filter_missing_labels_and_cancellation(tmp_path):
    root, _ = make_dataset(tmp_path, [("N3_M1_id1.cnf", "p cnf 3 1\n1 0\n", 1, "1 2 3")])
    make_dataset(tmp_path, [("N3_M1_id1.cnf", "p cnf 3 1\n-1 0\n", 1, "-1 2 3")], "train")
    train = SatDataset.from_randsatbench(root / "train")
    assert train.split == "train"
    assert analyze_dataset(train).runs.reference_valid.all()
    assert analyze_dataset(train).results == {}
    assert len(train.select(n=[3], alpha=(0.3, 0.4), limit=1).instances) == 1
    assert train.select(n=4).instances.empty
    token = CancellationToken()
    token.cancel()
    report = analyze_dataset(train, cancel=token)
    assert report.cancelled and report.runs.empty and report.summary().empty
    (root / "test" / "N3_M1_id2.cnf").write_text("p cnf 3 1\n1 0\n")
    report = analyze_dataset(root)
    missing = report.runs.loc[report.runs.name == "N3_M1_id2.cnf"].iloc[0]
    assert missing.reference_status == "UNKNOWN" and not missing.label_available


def test_bad_references_are_excluded_and_conflicts_flagged(tmp_path):
    root, _ = make_dataset(tmp_path, [
        ("N3_M1_id1.cnf", "p cnf 3 1\n1 0\n", 1, "-1 2 3 0"),
        ("N3_M1_id2.cnf", "p cnf 3 1\n1 0\n", 1, "1 1 3 0"),
        ("N3_M1_id3.cnf", "p cnf 3 1\n1 0\n", 0, ""),
    ])
    report = analyze_dataset(root)
    assert report.runs.verified.all()
    assert not report.runs.reference_valid.iloc[:2].any()
    assert report.runs.reference_error.iloc[:2].str.len().gt(0).all()
    assert report.runs.label_conflict.tolist() == [False, False, True]
    assert report.runs.agreement.isna().all()
    summary = report.summary(by=()).iloc[0]
    assert summary.invalid_reference_instances == 2
    assert summary.reference_sat_instances == 0 and np.isnan(summary.sat_recovery)
    assert summary.label_conflicts == 1


def test_input_errors_recorded_and_options_validated(tmp_path):
    root, _ = make_dataset(tmp_path, [
        ("N3_M1_id1.cnf", "p cnf 3 1\n1", 1, "1 2 3"),
        ("N3_M1_id2.cnf", "p cnf 4 1\n1 0\n", 1, "1 2 3 4"),
        ("N3_M1_id3.cnf", "p cnf 3 1\n1 0\n", 1, "1 2 3"),
    ])
    dataset = SatDataset.from_randsatbench(root)
    report = analyze_dataset(dataset)
    assert report.runs.status.tolist() == ["error", "error", "solved"]
    with pytest.raises(ValueError):
        analyze_dataset(dataset, SolverOptions(r=1))
    with pytest.raises(ValueError):
        analyze_dataset(dataset, seeds=[])
    with pytest.raises(ValueError):
        analyze_dataset(dataset, seeds=[0])
    with pytest.raises(ValueError):
        dataset.select(limit=-1)
    with pytest.raises(ValueError):
        dataset.select(alpha=(5, 3))


def test_label_validation_and_cooperative_partial_report(tmp_path):
    root, labels = make_dataset(tmp_path, [
        (f"N3_M1_id{i}.cnf", "p cnf 3 1\n1 0\n", 1, "1 2 3 0") for i in range(1, 4)
    ])
    token = CancellationToken()
    report = analyze_dataset(root, cancel=token, progress=lambda *args: token.cancel())
    assert report.cancelled and len(report.runs) == 1 and report.total_instances == 3
    with labels.open("a") as handle:
        handle.write("test-final/N3_M1_id1.cnf,1,1 2 3 0\n")
    with pytest.raises(ValueError, match="duplicate"):
        SatDataset.from_randsatbench(root)


@pytest.mark.parametrize("stop", ["complete", "cancel", "error"])
def test_progress_bar_reports_completed_trials_and_closes(tmp_path, capsys, stop):
    root, _ = make_dataset(tmp_path, [
        (f"N3_M1_id{i}.cnf", "p cnf 3 1\n1 0\n", 1, "1 2 3") for i in range(1, 3)
    ])
    token = CancellationToken()

    def callback(done, total, row):
        if stop == "cancel":
            token.cancel()
        elif stop == "error":
            raise RuntimeError("callback failed")

    if stop == "error":
        with pytest.raises(RuntimeError, match="callback failed"):
            analyze_dataset(root, seeds=[1, 2], cancel=token, progress=callback)
    else:
        report = analyze_dataset(root, seeds=[1, 2], cancel=token, progress=callback)
        assert len(report.runs) == (4 if stop == "complete" else 1)
    output = capsys.readouterr().err
    assert "BSP dataset" in output
    assert ("4/4" if stop == "complete" else "1/4") in output
    assert output.endswith("\n")


def test_disabled_bar_preserves_progress_callback(tmp_path, capsys):
    root, _ = make_dataset(tmp_path, [("N3_M1_id1.cnf", "p cnf 3 1\n1 0\n", 1, "1 2 3")])
    updates = []
    report = analyze_dataset(root, show_progress=False,
                             progress=lambda done, total, row: updates.append((done, total)))
    assert len(report.runs) == 1 and updates == [(1, 1)]
    assert capsys.readouterr().err == ""
