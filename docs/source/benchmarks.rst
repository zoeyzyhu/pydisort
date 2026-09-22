Performance
===========

pydisort exists because the accessible DISORT implementations are slow and the
fast ones are inaccessible. This page shows where it actually lands, and how to
reproduce the measurement on your own hardware.

What is measured
----------------

Spectral resolution is the dimension that makes radiative transfer expensive,
and it is the dimension pydisort parallelizes over: a single-column,
single-wavenumber solve is cheap, but a realistic calculation repeats it over
hundreds or thousands of spectral points.

Both benchmarks solve DISORT Test Problem 9 ("General Emitting/Absorbing/
Scattering") with **32 streams and 100 layers**, a workload typical of
atmospheric radiation calculations, and repeat it for a growing number of
wavenumbers. They answer two different questions, which is why they are
separate scripts:

.. list-table::
   :widths: 35 65
   :header-rows: 1

   * - Script
     - What it answers
   * - ``compare_cdisort.py``
     - What the Python wrapper costs. pydisort calls the same ``c_disort``, so
       the single-core ratio should be 1.0; the rest is what batching over
       wavenumbers buys. This is the durable result, since it does not depend
       on a third-party package's release.
   * - ``compare_pythonicdisort.py``
     - The interpreted/compiled gap, against `PythonicDISORT
       <https://doi.org/10.21105/joss.06442>`_ (Ho 2024), a pure-Python
       reimplementation.

Both import their problem definition from ``testproblem09.py``, so the thing
being timed is written down once rather than copied into each script.

Against cdisort
---------------

.. code-block:: bash

  python benchmarks/compare_cdisort.py --verify-only   # seconds
  python benchmarks/compare_cdisort.py                 # default sweep
  python benchmarks/compare_cdisort.py --nwave 1,10,100,1000,10000 \
      --threads 10 --repeat 3

No prepared build is needed. cdisort is header-only, so the script compiles its
own baseline from ``bench_cdisort.cpp`` with the repository's Release flags and
nothing but a C++17 compiler and libm.

Three properties of that baseline each move the number:

* **The solve loop is timed inside the C program.** Running a driver as a
  subprocess and subtracting an estimated process startup leaves a residual of
  about 0.7%, and it errs in the direction that flatters pydisort. Timing with
  ``std::chrono`` inside the loop removes the correction rather than modelling
  it.
* **The baseline is cross-validated against pydisort.** ``bench_cdisort.cpp``
  has a ``--verify`` mode, so the two are compared directly before any timing;
  they agree to :math:`\sim 10^{-23}`.
* **Allocation policy is a switch, not an assumption.**
  ``tests/cdisort213/test_cdisort_09.c`` allocates its ``disort_state`` on
  every iteration while pydisort allocates once per batch, so ``--alloc both``
  measures the difference rather than letting it hide inside the ratio. The
  two are within 0.3% of each other.

Result on an Apple M5 Max (18 logical CPUs), pydisort 1.8.5, torch 2.10.0,
Apple clang 21, best of 3, radiance mode:

.. code-block:: text

  Speed-up over cdisort (hoisted)
     nwave         1 core       10 cores
         1          0.98x          0.98x
        10          1.00x          6.35x
       100          1.00x          7.80x
      1000          1.00x          8.17x
     10000          1.01x          8.34x

  (same problem at 18 threads: 12.71x at nwave=1000, 13.25x at nwave=10000)

**The wrapper is free.** pydisort on one thread is 0.98 to 1.01x of cdisort
across four orders of magnitude of workload. That is the expected result, since
it calls the same solver on the same inputs, but it is worth pinning down
because it is exactly what a tensor-marshalling layer could plausibly get
wrong.

**Threading is where the gain is, and it is bounded by the core count.** On ten
threads the ratio saturates around 8.3x, which is 83% parallel efficiency;
eighteen threads reaches 13.3x. There is nothing to parallelize at one
wavenumber, so the single- and multi-threaded lines meet there and separate as
the spectral dimension fills the cores. Quote this ratio with the thread count
and the CPU, or it does not mean anything.

Against PythonicDISORT
----------------------

.. code-block:: bash

  pip install PythonicDISORT
  python benchmarks/compare_pythonicdisort.py --verify-only
  python benchmarks/compare_pythonicdisort.py --nwave 1,10,100,1000 \
      --threads 10 --compare-modes

Same machine, PythonicDISORT 1.8, radiance mode:

.. list-table::
   :widths: 20 40 40
   :header-rows: 1

   * - nwave
     - vs pydisort (1 core)
     - vs pydisort (10 cores)
   * - 1
     - 42.0x
     - 42.4x
   * - 100
     - 40.8x
     - 314.8x
   * - 1000
     - 39.4x
     - 332.1x
   * - 10000
     - 39.9x
     - 334.7x

The single-core ratio is flat at about 40x, which is the pure interpreter
overhead and is independent of problem size. The multi-threaded ratio climbs
while the cores fill and then saturates near 335x by about 1000 wavenumbers.

.. note::

   The sweep above stops at 1000 because the ratio has stopped moving by then.
   Extending it to 10000, as the paper's figure does, costs about nine minutes
   on this machine and closer to an hour on the M1 Max the paper used, almost
   all of it PythonicDISORT. ``compare_pythonicdisort.py`` projects the
   runtime from one solve and prints it before starting, so a long sweep is a
   choice rather than a surprise. ``compare_cdisort.py`` has no such problem:
   the same point costs about 30 seconds there.

.. warning::

   Do not quote this ratio as a property of pydisort. It is a ratio between two
   moving targets: PythonicDISORT's own performance has improved across
   releases, and the two codes do not scale identically across hardware, so the
   same measurement on a different machine or a different PythonicDISORT
   version can land a factor of two or three away. The durable results are the
   comparisons against cdisort above.

Getting the most out of pydisort
--------------------------------

**Batch, do not loop.** Fill a ``(nwave, ncol, nlyr, nprop)`` tensor and make
one ``forward`` call rather than looping over wavelengths or columns. This is
the single most important thing for performance; see :doc:`usage`. The
examples in :doc:`examples` batch along both axes.

**Use** ``onlyfl`` **when you only need fluxes.** Skipping the radiance
computation is substantially cheaper.

**Control the thread count** with ``torch.set_num_threads(n)``. The default is
usually reasonable, but when pydisort runs inside an already-parallel
application, oversubscription can cost more than it gains.

Other benchmarks
----------------

The ``benchmarks/`` directory also contains:

* ``benchmark_cuda_fp64.py`` — FP64 flux-only solvers on one CPU thread and on
  CUDA, optionally against Exo-FMS Fortran Toon solvers, through the Fortran
  driver ``benchmark_exofms_toon.f90`` and its build script;
* ``benchmark_cuda_agreement.py`` — numerical agreement between the CPU and
  CUDA solver paths;
* ``validate_fast_flux_routing.py`` — checks that the specialised fast
  flux-only paths agree with the general solver.

See `benchmarks/README.md
<https://github.com/zoeyzyhu/pydisort/blob/main/benchmarks/README.md>`_ for
details.
