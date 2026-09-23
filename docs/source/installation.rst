Installation and Quickstart
===========================

Install
-------

pydisort is published on `PyPI <https://pypi.org/project/pydisort/>`_ with
prebuilt binary wheels, so no compiler or Fortran toolchain is needed on a
matching platform:

.. code-block:: bash

  python -m pip install pydisort

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
   * - Linux wheels
     - x86-64, glibc 2.28 or newer (including the PyTorch dependency)
   * - macOS wheels
     - Apple Silicon (arm64), macOS 15 or newer
   * - Dependencies
     - ``numpy``, ``torch`` (installed automatically)

The release workflow targets these platforms; consult the
`PyPI download files <https://pypi.org/project/pydisort/#files>`_ for available
wheels. The workflow does not build wheels for Windows, Intel macOS, Linux ARM
or free-threaded Python. A source build is not a guarantee of support on those
targets.

The glibc floor of 2.28 is inherited from PyTorch v2.7 and later. Wheels are
built and published automatically with ``cibuildwheel`` through GitHub Actions.

Installing from source
~~~~~~~~~~~~~~~~~~~~~~

PyPI releases publish wheels, not a source distribution. If no wheel matches,
``pip install pydisort`` cannot automatically fall back to a source build.
Build from a repository checkout instead. Use the same activated Python
environment for CMake and pip:

.. code-block:: bash

  git clone https://github.com/zoeyzyhu/pydisort.git
  cd pydisort
  python3 -m venv env
  source env/bin/activate
  python -m pip install --upgrade pip
  python -m pip install 'torch==2.10.0' numpy 'cmake>=3.20' ninja \
      setuptools 'setuptools-scm>=8' wheel
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
  cmake --build build --parallel
  python -m pip install --no-build-isolation .

This builds the CPU configuration. A C++17 compiler is required (GCC 9 or
newer on Linux, or a compatible Apple Clang on macOS). CMake builds the C++
library first; ``setup.py`` only builds the Python bindings and links that
library. Build isolation is disabled to keep both stages on the same PyTorch
installation. To reproduce a release, check out its tag before building.

Verify the installation
-----------------------

.. code-block:: python

  >>> import pydisort
  >>> pydisort.__version__               # doctest: +SKIP
  '1.8.5'

The quickstart below works directly with an installed wheel. For the
Beer-Lambert example and the test suite, first obtain the repository files:

.. code-block:: bash

  git clone https://github.com/zoeyzyhu/pydisort.git
  cd pydisort
  python examples/example_01_beam_attenuation.py

Use a checkout matching your installed release when validating a release.
The example ends with ``OK: direct beam reproduces the Beer-Lambert law to
machine precision.`` See :doc:`testing` for the full test suite.

Quickstart
----------

Every pydisort program has the same two steps: describe the problem with a
:class:`pydisort.DisortOptions` object, then run it by calling
:meth:`~pydisort.Disort.forward` on a tensor of optical properties.

.. include:: _snippets/quickstart.rst

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

The full list is documented on :class:`pydisort.DisortOptions`. Some backend
flags are recognized but not supported through Python; see
:ref:`python-flag-support` before enabling additional flags.

Where to go next
----------------

* :doc:`usage`, input/output shapes, batching and singleton dimensions.
* :doc:`examples`, four complete, self-checking calculations, ending with a
  real-world solver-validation study.
* :doc:`api`, the full API reference.
* :doc:`testing`, how to run and extend the test suite.
* :doc:`benchmarks`, performance against cdisort and PythonicDISORT.

Building the C++ library
------------------------

pydisort also ships a C++ API for embedding the solver in larger C or C++
simulation frameworks. Building it requires ``cmake`` (>= 3.20), a C++17
compiler and Python 3.10 or newer. Use the environment and source-build
procedure above. To enable the C, C++ and Python test targets afterwards:

.. code-block:: bash

  python -m pip install pytest
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
  cmake --build build --parallel
  python -m pip install --no-build-isolation .
  ctest --test-dir build --output-on-failure

See :doc:`contribute` for the contributor workflow.

Getting help
------------

* Bug reports and feature requests:
  `GitHub issues <https://github.com/zoeyzyhu/pydisort/issues>`_
* Common runtime errors are covered in :ref:`troubleshooting`.
