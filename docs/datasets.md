# Dataset pipelines

`Dataset` combines an indexed metadata table with lazy formula records. It
shares its implementation with the `SatDataset` name used by the benchmark
interface. Metadata operations evaluate immediately. Clauses are loaded or
generated when a formula is needed.

## Chain filtering and stratified sampling

```python
import pybsp

DATASET_ROOT = "~/workspace/RandSatBench/datasets/3SAT"
dataset = (
    pybsp.Dataset.from_randsatbench(DATASET_ROOT, split="train")
    .select(n=[16, 32], alpha=(4.0, 4.3))
    .sample(num_instances=10, by=["N", "alpha"], seed=123)
    .sort(["N", "alpha", "name"])
)
dataset.head().to_frame()
```

`sample(num_instances=10)` samples ten rows overall. Adding `by=["N", "alpha"]`
samples ten **per group**. `sample(frac=0.1)` samples a fraction. Supply exactly
one of count/fraction. Sampling is without replacement and raises if a group is
too small, preserving the intended experimental balance.

| Operation | Result |
|---|---|
| `filter(lambda row: row["N"] == 32)` | Predicate over metadata dictionaries |
| `query("N == 32 and alpha >= 4.1")` | Vectorized metadata expression |
| `head(5)`, `tail(5)` | First/last rows as datasets |
| `select([0, 2, 4])` | Positional selection |
| `shuffle(seed=123)`, `sort("alpha")` | Rearranged dataset |
| `shard(4, 0)` | Every fourth row, starting at position zero |
| `train_test_split(test_size=0.2, seed=123)` | Disjoint `train`/`test` datasets |
| `pipe(function)` | Custom operation receiving this dataset |
| `to_frame()` | Independent metadata DataFrame |

Operations return new datasets. Split subsets preserve the original source/label
split. Use an explicit experiment partition column in reports. `num_rows`, `shape`
and `column_names` describe the metadata view. Indexing an item returns a
`DatasetInstance`. Call `item.load()` to obtain a formula. `iter_batches()` groups
lazy records without loading them.

## Reference labels

RandSATBench CSV columns are `cnf_file,sat,assignment`, with `1=SAT`, `0=UNSAT`,
and `-1`/blank=UNKNOWN. Label files are discovered beside the split/root or in the
repository's `3SAT/`/`4SAT/` directory. Override with `labels_csv=...` when needed.
Old `train-final`/`test-final` prefixes match by filename within the chosen split.
Missing labels remain UNKNOWN. Signed reference assignments may end in zero.

Every SAT reference and returned BSP assignment is checked against original
clauses. A different satisfying assignment is equally correct. Hamming distance
and exact match describe similarity between assignments. Solver failure never
turns an UNKNOWN reference into UNSAT.

## Generate a reproducible random ensemble

```python
dataset = pybsp.Dataset.from_random_ksat(
    n=32, alpha=4.0, k=3, num_instances=20, seed=123,
)
labelled = dataset.with_solutions(pybsp.SolverOptions(seed=99), show_progress=False)
```

`m` can specify an exact clause count instead of `alpha`. If both are supplied,
`m` must equal `ceil(alpha*n)`. Recorded density is the actual `m/n`. Generation
seeds are separate from solver trajectory seeds. Omitting a solver keeps creation
lazy. Supplying `solver=` to the constructor computes reference solutions.
Dataset generation seeds may be zero. Explicit native solver trajectory seeds
must be positive.

Reference solvers may be `SolverOptions`, a configured `KSATSolver`, or a callable
`Formula -> assignment/None`. Callable solvers currently run serially. A solver
object uses its own execution settings. Invalid assignments raise. Unsuccessful
runs preserve existing labels. `reference_solver_status` records outcomes and
`dataset.info` records generation/reference provenance. Heuristic references
identify verified SAT instances but do not determine which remaining instances
are UNSAT.

## Save and load a portable snapshot

```python
# Explicit persistence to a new/empty directory:
# labelled.save_to_disk("experiments/labelled-ksat")
# restored = pybsp.Dataset.load_from_disk("experiments/labelled-ksat")
```

Snapshots contain `dataset.json` (format/provenance), `metadata.json` (index),
and `formulas.jsonl` (streamed clause data). Loading reads metadata. Indexed file
offsets retrieve formulas as needed. References and random RNG replay information
are preserved. The directory can be moved independently of the original CNFs.
This format is specific to pybsp. Use `from_randsatbench()` for benchmark directories.
