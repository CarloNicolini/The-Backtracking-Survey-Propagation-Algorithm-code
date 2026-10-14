KSATSolver
==========

.. autoclass:: pybsp.KSATSolver
   :members: fit_predict, fit, predict, evaluate, score, get_params, set_params

``random_state`` replaces ``SolverOptions.seed``. It accepts an integer or
``None``. Explicit native trajectory seeds must be positive. Execution controls are ``n_jobs``, ``batch_size``, ``keep_results`` and
``show_progress``. Other constructor arguments mirror ``SolverOptions``.

After execution:

* ``assignments_`` is an ordered list of signed arrays or ``None``.
* ``statuses_`` is an array of run outcomes.
* ``run_table_`` is a DataFrame copy of the measurements.
* ``report_`` is the complete ``BatchResult``.

``fit`` and ``predict`` each perform a new search. ``fit_predict`` is the usual
entry point. Repeated calls replace earlier outputs. ``y`` is accepted for
interoperability but never guides search. Dataset references provide labels.
Cloning and parameter utilities are supported, but CNFs are not ordinary feature
vectors and assignments are not classification targets.
