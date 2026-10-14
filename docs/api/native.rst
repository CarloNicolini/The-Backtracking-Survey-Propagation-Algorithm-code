Formula and native runs
=======================

.. autoclass:: pybsp.Formula
   :members:

.. autoclass:: pybsp.SolverOptions
   :members:

Options use the executable's defaults and validation. CLI hyphens become
underscores. See :doc:`../python-native` for option mapping and combinations.
``oracle_pick > 0`` is explicitly unsupported.

.. autofunction:: pybsp.solve

.. autoclass:: pybsp.SolveResult
   :members:

``SolveResult`` is returned by ``solve``. Its constructor is not a user-facing
input format. ``status``, ``reason``, ``seed`` and ``wall_seconds`` describe the
run. ``solutions``, ``surveys`` and optional ``populations`` are owned arrays.
``stdout``, ``stderr`` and ``logs`` retain output. ``tables`` provides named pandas
frames. Residual histories and population snapshots require opt-in settings.

.. autoclass:: pybsp.SolverError

.. autoclass:: pybsp.LogRecord
   :members:

.. py:class:: pybsp.CancellationToken

   A process-local cooperative cancellation request. Batch workers bridge the
   parent's token to their local tokens.

   .. py:method:: cancel()

      Request cancellation. A requested token cannot be reset. Create a new one
      for a later run.

   .. py:attribute:: requested

      Read-only boolean indicating that cancellation was requested.
