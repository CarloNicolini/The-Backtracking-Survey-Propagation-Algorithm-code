Datasets
========

``Dataset`` and ``SatDataset`` name the same class. Prefer factory constructors
to manually assembling the metadata DataFrame.

.. autoclass:: pybsp.Dataset
   :members:
   :inherited-members:
   :special-members: __len__, __iter__, __getitem__

``info`` stores generation/reference solver provenance. ``labels_csv`` identifies
an imported label file. Portable snapshots do not depend on that original file.
``instances`` remains available for compatibility. Use chained methods and
``to_frame`` for routine experiments.

.. autoclass:: pybsp.DatasetInstance
   :members:

Records contain metadata and source split information. ``load()`` reads or
generates their formula without loading unrelated instances.
