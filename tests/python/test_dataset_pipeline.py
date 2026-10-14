import shutil
from pathlib import Path

import numpy as np
import pandas as pd
import pytest

from pybsp import Dataset, Formula, SolverOptions, solve_batch
from test_benchmark import make_dataset


def fixture_dataset(tmp_path):
    root, _ = make_dataset(tmp_path, [
        (f'N{n}_M1_id{i}.cnf', f'p cnf {n} 1\n1 0\n', 1, ' '.join(map(str,range(1,n+1))))
        for n in [3,4] for i in range(1,7)
    ])
    return Dataset.from_randsatbench(root)


def test_metadata_pipeline_is_lazy_and_preserves_source(tmp_path, monkeypatch):
    dataset = fixture_dataset(tmp_path)
    original = dataset.to_frame(include_assignments=True)
    monkeypatch.setattr(Formula, 'from_file', lambda *args: pytest.fail('metadata loaded a CNF'))
    result = (dataset.filter(lambda row: row['N'] == 3).sort('name', ascending=False)
              .sample(num_instances=3,seed=123).head(2))
    assert len(result) == 2 and result.to_frame().N.eq(3).all()
    assert dataset.to_frame(include_assignments=True).equals(original)
    assert dataset.head().num_rows == dataset.tail().num_rows == 5
    assert len(dataset.head(0)) == len(dataset.tail(0)) == 0
    assert dataset.shape == dataset.to_frame().shape
    assert dataset.column_names == dataset.to_frame().columns.tolist()
    assert len(dataset.query('N == 4')) == 6
    assert dataset.select([2,0]).to_frame().name.tolist() == original.iloc[[2,0]].name.tolist()
    assert dataset.pipe(lambda ds: ds.head(2)).num_rows == 2


def test_stratified_sampling_shards_and_splits(tmp_path):
    dataset = fixture_dataset(tmp_path)
    a = dataset.sample(num_instances=2,by=['N','alpha'],seed=3)
    b = dataset.sample(num_instances=2,by=['N','alpha'],seed=3)
    assert a.to_frame().equals(b.to_frame())
    assert a.to_frame().groupby('N').size().tolist() == [2,2]
    assert len(dataset.sample(frac=0.5,seed=3)) == 6
    shards = [dataset.shard(3,i).to_frame() for i in range(3)]
    assert sum(map(len,shards)) == len(dataset)
    assert pd.concat(shards).name.nunique() == len(dataset)
    split = dataset.train_test_split(test_size=3,seed=123)
    assert len(split['test']) == 3 and len(split['train']) == 9
    assert set(split['test'].to_frame().name).isdisjoint(split['train'].to_frame().name)
    assert dataset.train_test_split(0.25,seed=123)['test'].to_frame().equals(split['test'].to_frame())
    with pytest.raises(ValueError): dataset.sample(frac=0.5,num_instances=2)
    with pytest.raises(ValueError): dataset.sample(num_instances=7,by='N')
    with pytest.raises(ValueError): dataset.head(-1)
    with pytest.raises(TypeError): dataset.filter(lambda row: 1)
    with pytest.raises(IndexError): dataset.select([len(dataset)])


def test_random_generation_laziness_and_exact_clause_count(monkeypatch):
    dataset = Dataset.from_random_ksat(n=50,m=29,k=3,num_instances=4,seed=17)
    expected = dataset[0].load()
    assert len(expected.clauses) == 29
    assert dataset[0].load() == expected
    assert Dataset.from_random_ksat(50, m=29, num_instances=4, seed=17)[0].load() == expected
    assert dataset.instances.generation_seed.nunique() == 4
    monkeypatch.setattr(Formula,'random',lambda **kw: pytest.fail('metadata generated a formula'))
    new = Dataset.from_random_ksat(50,alpha=0.58,m=29,num_instances=10).shuffle(3).head(3)
    assert len(new) == 3 and new.to_frame().M.eq(29).all()
    for kwargs in [{'n':2,'k':3,'m':1},{'n':3,'m':-1},{'n':3,'m':3,'alpha':2},
                   {'n':3,'alpha':np.nan},{'n':3,'m':2,'num_instances':-1}]:
        with pytest.raises(ValueError): Dataset.from_random_ksat(**kwargs)


def test_random_reference_solutions_and_parallel_loading(tmp_path):
    dataset = Dataset.from_random_ksat(8,alpha=1,k=3,num_instances=5,seed=19,
                                      solver=SolverOptions(seed=2),show_progress=False)
    assert dataset.to_frame().reference_status.eq('SAT').all()
    assert dataset.info['reference_solver']['options']['seed'] == 2
    a = solve_batch(dataset,show_progress=False)
    b = solve_batch(dataset,n_jobs=2,batch_size=1,show_progress=False)
    assert a.runs.reference_valid.all() and b.runs.reference_valid.all()
    for x,y in zip(a.assignments,b.assignments): assert np.array_equal(x,y)
    assert list(tmp_path.iterdir()) == []
    empty = Dataset.from_random_ksat(3,m=0,num_instances=2)
    labelled = empty.with_solutions(lambda f: np.ones(f.n,dtype=bool))
    assert labelled.to_frame().reference_status.eq('SAT').all()
    assert empty.to_frame().reference_status.eq('UNKNOWN').all()
    unknown = empty.with_solutions(lambda f: None)
    assert unknown.to_frame().reference_status.eq('UNKNOWN').all()
    with pytest.raises(ValueError): empty.with_solutions(lambda f: [1])


def test_portable_snapshot_roundtrip_without_regeneration(tmp_path,monkeypatch):
    dataset = Dataset.from_random_ksat(8,m=8,num_instances=3,seed=19,
                                      solver=SolverOptions(seed=2),show_progress=False)
    formulas = [item.load() for item in dataset]
    saved = dataset.save_to_disk(tmp_path/'snapshot')
    assert sorted(p.name for p in saved.iterdir()) == ['dataset.json','formulas.jsonl','metadata.json']
    shutil.move(saved,tmp_path/'moved')
    monkeypatch.setattr(Formula,'random',lambda **kw: pytest.fail('snapshot regenerated formula'))
    loaded = Dataset.load_from_disk(tmp_path/'moved')
    assert [item.load() for item in loaded] == formulas
    assert loaded.info == dataset.info
    report = solve_batch(loaded,show_progress=False)
    assert report.runs.reference_valid.all()
    assert loaded.sample(num_instances=2,seed=3).num_rows == 2
    with pytest.raises(FileExistsError): dataset.save_to_disk(tmp_path/'moved')


def test_disk_snapshot_is_independent_of_original_cnfs(tmp_path):
    dataset = fixture_dataset(tmp_path).head(2)
    formulas = [item.load() for item in dataset]
    directory = dataset.save_to_disk(tmp_path/'snapshot')
    for item in dataset:
        Path(item.metadata['path']).unlink()
    loaded = Dataset.load_from_disk(directory)
    assert [item.load() for item in loaded] == formulas
    assert solve_batch(loaded,show_progress=False).runs.reference_valid.all()


def test_no_solution_preserves_existing_references(tmp_path):
    dataset = fixture_dataset(tmp_path).head(2)
    labelled = dataset.with_solutions(lambda formula: None)
    assert labelled.to_frame().reference_status.eq('SAT').all()
    assert labelled.to_frame(include_assignments=True).reference_assignment.tolist() == (
        dataset.to_frame(include_assignments=True).reference_assignment.tolist())


def test_empty_random_snapshot_and_zero_sampling(tmp_path):
    dataset = Dataset.from_random_ksat(n=3,m=0,num_instances=0)
    assert dataset.head().to_frame().empty
    assert dataset.sample(frac=1).num_rows == 0
    path = dataset.save_to_disk(tmp_path/'empty')
    assert Dataset.load_from_disk(path).num_rows == 0
