Pydisort Documentation
======================

**A modern Python package for DISORT radiative transfer, with support for
parallel computation.**

pydisort provides a Python interface to the C version of the DISORT
(Discrete Ordinates Radiative Transfer) program. It wraps the well-tested
``cdisort`` numerical core in a C++ class (``DisortWrapper``), which is in turn
bound to Python with pybind11, and uses PyTorch tensors as its primary data
structure.

Please consult the DISORT publication [1]_ for more information on the DISORT
program, and the C-DISORT publication [2]_ for the C version.

.. code-block:: bash

  pip install pydisort

Prebuilt wheels are published for CPython 3.10-3.14 on Linux and macOS, so no
compiler or Fortran toolchain is required. See :doc:`installation` to get
started.

Why pydisort?
-------------

pydisort features the following benefits over the original C-DISORT program:

- **Parallel by design.** Wavelength and atmospheric column are batch
  dimensions, so a spectral or multi-column calculation spreads across cores.
  On one thread pydisort matches cdisort exactly; with ten threads it is
  roughly an order of magnitude faster (see :doc:`benchmarks`).
- **PyTorch-native**, so radiative transfer drops directly into tensor-based
  scientific and machine-learning workflows.
- **No build step.** Prebuilt binary wheels on PyPI, rather than a local
  Fortran or C compilation.
- **Proper error handling**, rather than abrupt exit of the program. Errors
  can be caught and handled in the Python script.
- **Automatic memory management**, handled by the C++ class. The user does not
  need to worry about allocation and deallocation.
- **Safety guards** that prevent setting incorrect values for arrays or
  calling methods in the wrong order.
- **Documented and tested**, with documentation automated through Sphinx and
  Read the Docs, and a test suite validated against published DISORT reference
  values (see :doc:`testing`).

Note that the underlying calculation engine is still the same as the C-DISORT
program, so results agree with cdisort to near machine precision.

:doc:`statement_of_need` explains in more detail what problem pydisort solves,
who it is for, and how it relates to the other DISORT implementations.

A 30-second example
-------------------

Every pydisort program has the same two steps: describe the problem with a
:class:`pydisort.DisortOptions` object, then run it by calling
:meth:`~pydisort.Disort.forward` on a tensor of optical properties.

.. code-block:: python

  >>> import torch
  >>> from pydisort import Disort, DisortOptions
  >>>
  >>> op = DisortOptions().flags("onlyfl,lamber")
  >>> op.ds().nlyr = 4
  >>> op.ds().nstr = 4
  >>> op.ds().nmom = 4
  >>> op.ds().nphase = 4
  >>>
  >>> ds = Disort(op)
  >>> tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).unsqueeze(-1)
  >>> ds.forward(tau, fbeam=torch.tensor([3.14159]))
  tensor([[[[0.0000, 3.1416],
          [0.0000, 2.8426],
          [0.0000, 2.3273],
          [0.0000, 1.7241],
          [0.0000, 1.1557]]]])

The returned tensor has shape ``(nwave, ncol, nlvl, 2)``: wavelength, column,
level, then upward and downward flux. :doc:`usage` explains those dimensions.

Where to go next
----------------

.. list-table::
   :widths: 30 70

   * - :doc:`installation`
     - Install pydisort, verify it works, and run your first calculation.
   * - :doc:`statement_of_need`
     - What problem pydisort solves, who it is for, and its scope.
   * - :doc:`usage`
     - Input and output dimensions, broadcasting, flags, troubleshooting.
   * - :doc:`examples`
     - Four complete calculations, ending with a real-world solver-validation
       study.
   * - :doc:`api`
     - Full API reference.
   * - :doc:`testing`
     - How the package is validated, and how to run the test suite.
   * - :doc:`benchmarks`
     - Performance against cdisort and PythonicDISORT, and how to reproduce it.
   * - :doc:`contribute`
     - Contributor workflow.

References
----------
.. [1] Stamnes, K., Tsay, S. C., Wiscombe, W., & Jayaweera, K. (1988).
       Numerically stable algorithm for discrete-ordinate-method radiative transfer in multiple scattering and emitting layered media.
       Applied Optics, 27(12), 2502-2509.
.. [2] Buras, R., & Dowling, T. (1996).
       Discrete-ordinate-method for radiative transfer in planetary atmospheres: Generalization of the doubling and adding method.
       Journal of Quantitative Spectroscopy and Radiative Transfer, 55(6), 761-779.

.. toctree::
    :maxdepth: 2
    :caption: Getting started

    installation
    statement_of_need
    usage

.. toctree::
    :maxdepth: 2
    :caption: Using pydisort

    examples
    api
    type_hints

.. toctree::
    :maxdepth: 2
    :caption: Validation

    testing
    benchmarks

.. toctree::
    :maxdepth: 2
    :caption: Development

    contribute
    venv
    devops
