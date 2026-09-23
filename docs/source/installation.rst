Installation and Quickstart
===========================

Install
-------

pydisort is published on `PyPI <https://pypi.org/project/pydisort/>`_ with
prebuilt binary wheels, so no compiler and no Fortran toolchain are needed:

.. code-block:: bash

  pip install pydisort

``pip`` pulls in a compatible ``torch`` automatically.

Supported platforms
~~~~~~~~~~~~~~~~~~~

.. list-table::
   :widths: 30 70
   :header-rows: 1

   * - Requirement
     - Supported
   * - Python
     - CPython 3.10, 3.11, 3.12, 3.13, 3.14
   * - Operating system
     - Linux (glibc 2.28 or newer) and macOS
   * - Dependencies
     - ``numpy``, ``torch`` (installed automatically)

The glibc floor of 2.28 is inherited from PyTorch v2.7 and later. Wheels are
built and published automatically with ``cibuildwheel`` through GitHub Actions.

Installing from source
~~~~~~~~~~~~~~~~~~~~~~

If ``pip`` finds no matching wheel it falls back to building from the source
distribution. pydisort is a compiled PyTorch extension, so ``torch`` must be
importable *at build time*. Install it first and disable build isolation:

.. code-block:: bash

  pip install 'torch==2.10.0'
  pip install pydisort --no-build-isolation

Verify the installation
-----------------------

.. code-block:: python

  >>> import pydisort
  >>> pydisort.__version__               # doctest: +SKIP
  '1.8.5'

For an end-to-end check that also validates the numerics, run the first
example, which compares the solver against the analytic Beer-Lambert solution:

.. code-block:: bash

  python examples/example_01_beam_attenuation.py

It ends with ``OK: direct beam reproduces the Beer-Lambert law to machine
precision.`` See :doc:`testing` for the full test suite.

Quickstart
----------

Every pydisort program has the same two steps: describe the problem with a
:class:`pydisort.DisortOptions` object, then run it by calling
:meth:`~pydisort.Disort.forward` on a tensor of optical properties.

.. code-block:: python

  >>> import torch
  >>> from pydisort import Disort, DisortOptions
  >>>
  >>> # 1. describe the problem
  >>> op = DisortOptions().flags("onlyfl,lamber")
  >>> op.ds().nlyr = 4      # atmospheric layers
  >>> op.ds().nstr = 4      # discrete-ordinate streams
  >>> op.ds().nmom = 4      # phase-function moments
  >>> op.ds().nphase = 4
  >>>
  >>> # 2. build the solver and run it
  >>> ds = Disort(op)
  >>> tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).unsqueeze(-1)
  >>> flx = ds.forward(tau, fbeam=torch.tensor([3.14159]))
  >>> flx
  tensor([[[[0.0000, 3.1416],
          [0.0000, 2.8426],
          [0.0000, 2.3273],
          [0.0000, 1.7241],
          [0.0000, 1.1557]]]])

The returned tensor has shape ``(nwave, ncol, nlvl, 2)``: wavelength, column,
level, and then upward and downward flux. :doc:`usage` explains those
dimensions in detail, and :doc:`examples` works through complete calculations.

.. tip::

  DISORT is a double-precision code. Call
  ``torch.set_default_dtype(torch.float64)`` at the top of your script so that
  accuracy is not limited by float32.

Common flags
------------

Flags are passed as one comma-separated string to
:meth:`~pydisort.DisortOptions.flags`. The ones needed most often are:

.. list-table::
   :widths: 25 75
   :header-rows: 1

   * - Flag
     - Meaning
   * - ``lamber``
     - Lambertian lower boundary. **Almost always required**; without it
       DISORT looks for a BRDF model and aborts.
   * - ``onlyfl``
     - Compute fluxes only, skipping radiances. This is the fast path; use it
       unless you need directional intensities.
   * - ``planck``
     - Enable thermal emission. Requires a level temperature profile (``temf``)
       and the spectral limits of each band.
   * - ``usrang``
     - Return radiances at user-specified viewing angles.
   * - ``usrtau``
     - Return results at user-specified optical depths rather than at layer
       boundaries.
   * - ``quiet``
     - Suppress cdisort's internal printout.

The full list is documented on :class:`pydisort.DisortOptions`.

Where to go next
----------------

* :doc:`usage`, the input and output dimensions, and how broadcasting works.
* :doc:`examples`, four complete, self-checking calculations, ending with a
  real-world solver-validation study.
* :doc:`api`, the full API reference.
* :doc:`testing`, how to run and extend the test suite.
* :doc:`benchmarks`, performance against cdisort and PythonicDISORT.

Building the C++ library
------------------------

pydisort also ships a C++ API for embedding the solver in larger C or C++
simulation frameworks. Building it requires ``cmake`` (>= 3.18), a C++17
compiler and Python 3.10 or newer. CMake resolves PyTorch through
``find_package(Torch REQUIRED)``, so ``torch`` has to be importable in the
active environment before you configure:

.. code-block:: bash

  git clone https://github.com/zoeyzyhu/pydisort.git
  cd pydisort
  pip install 'torch==2.10.0'
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
  cmake --build build
  ctest --test-dir build

See the repository ``README.md`` for the full C++ developer instructions, and
:doc:`contribute` for the contributor workflow.

Getting help
------------

* Bug reports and feature requests:
  `GitHub issues <https://github.com/zoeyzyhu/pydisort/issues>`_
* Common runtime errors are covered in :ref:`troubleshooting`.
