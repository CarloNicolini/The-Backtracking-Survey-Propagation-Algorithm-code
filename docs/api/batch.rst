Batch solving and analysis
==========================

.. autofunction:: pybsp.solve_batch

.. autofunction:: pybsp.iter_solve

``solve_batch`` forwards keyword arguments to ``iter_solve``: ``seeds``,
``n_jobs``, ``batch_size``, ``keep_results``, ``show_progress``, and ``cancel``.
Ordered assignment lists align with report rows, including ``None`` on failure.
For an unsized generator, ``total_instances`` is ``None``.

.. autoclass:: pybsp.BatchResult
   :members:
   :inherited-members:

``results`` is keyed by ``(input_index, trial)`` when retention is enabled.
``manifest`` holds execution metadata. ``metrics()`` and ``summary()`` reuse
existing results. Neither method runs the solver again.

.. autofunction:: pybsp.analyze_dataset

.. autoclass:: pybsp.DatasetAnalysis
   :members:

``analyze_dataset`` is the serial labelled-dataset convenience API, with an
optional per-trial ``progress`` callback and logger forwarding. Its retained
results are keyed by ``(filename, trial)``. Use ``solve_batch`` for processes.
