Automated Tests
===============

pydisort's correctness rests on the fact that DISORT has published reference
results. The test suite checks the Python bindings against those references,
against analytic solutions, and against conservation laws, so that a change in
the wrapper, the build, or the C backend cannot silently alter the numbers.

Running the tests
-----------------

From a checkout of the repository
`<https://github.com/zoeyzyhu/pydisort>`_:

The Python tests only need an installed pydisort:

.. code-block:: bash

  pip install pydisort pytest
  pytest tests/ -v

The full suite, including the C and C++ tests, is driven by CTest and requires
a build:

.. code-block:: bash

  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
  cmake --build build
  ctest --test-dir build --output-on-failure

To include the examples as tests, configure with ``-DBUILD_EXAMPLES=ON``; they
are also runnable directly through pytest (see below).

What is tested
--------------

The suite is in two halves, which fail for different reasons. A failure under
``tests/reference/`` means the physics moved; a failure in the modules beside
it means the wrapper moved.

Reference conformance (``tests/reference/``)
""""""""""""""""""""""""""""""""""""""""""""

Ports of the test problems distributed with DISORT, checked against the values
published with them. Every module builds its cases from the shared ``solve``
fixture in ``conftest.py``, so they differ only in their inputs.

.. list-table::
   :widths: 34 10 56
   :header-rows: 1

   * - File
     - Cases
     - What it checks
   * - ``test_problem_01_isotropic.py``
     - 6
     - **Isotropic scattering** in a single layer. Thin
       (:math:`\tau = 0.03125`) and thick (:math:`\tau = 32`) media,
       single-scattering albedos of 0.2, 0.99 and 1.0, under both beam and
       diffuse illumination.
   * - ``test_problem_02_rayleigh.py``
     - 4
     - **Rayleigh scattering**. The first phase function with angular
       structure, so the Legendre expansion starts to matter.
   * - ``test_problem_03_henyey_greenstein.py``
     - 3
     - **Forward-peaked scattering** at :math:`g = 0.75`, with 32 moments
       against 16 streams. The only problem that needs more moments than
       streams, which is what makes DISORT's delta-M truncation run.
   * - ``test_problem_06_lambertian_surface.py``
     - 4
     - **A reflecting surface.** Every other reference problem uses a black
       lower boundary, so this is the only one that exercises Lambertian
       reflection. Also covers the transparent limit, :math:`\tau = 0`.
   * - ``test_problem_09_inhomogeneous.py``
     - 5
     - **Six layers, every one different**, and the only problem where the
       layer-to-layer interface matching runs at all. Case 9c adds thermal
       emission, a reflecting surface, a per-layer asymmetry parameter and
       three azimuths at once.
   * - ``test_problem_10_user_vs_quadrature.py``
     - 3
     - **Reporting angles do not affect fluxes.** The same medium solved with
       ``usrang`` set and clear, which is a different output path: intensities
       are produced directly rather than interpolated.

Upstream ships fourteen problems and six are ported here. The rest were left
out deliberately: 4 and 5 add further tabulated phase functions but no new
capability, 7 and 12 depend on a BRDF, which pydisort does not expose
(``brdf_type`` is fixed at ``BRDF_NONE``), 8 and 11 are multi-layer cases
already covered by 9, 13 uses ``ibcnd``, which ``forward`` rejects, and 14
compares against ``twostr``, a separate solver.

Behaviour (``tests/``)
""""""""""""""""""""""

What pydisort adds on top of cdisort, and the contracts its API makes.

.. list-table::
   :widths: 34 10 56
   :header-rows: 1

   * - File
     - Cases
     - What it checks
   * - ``test_attenuation.py``
     - 1
     - Beam attenuation in a purely absorbing atmosphere, against the
       Beer-Lambert law.
   * - ``test_batching.py``
     - 9
     - **Batching over columns and wavelengths.** cdisort solves one column at
       one wavelength; pydisort dispatches over both axes at once. Every
       element of a batch must reproduce exactly what it produces alone.
   * - ``test_output_accessors.py``
     - 9
     - **forward, gather_flx and gather_rad agree.** Same level count, same
       fluxes, same ordering, including under the ``upward`` option and for
       output grids coarser and finer than the layer boundaries.
   * - ``test_scattering_moments.py``
     - 39
     - **All six phase functions.** The analytic forms against their closed
       form, the two tabulated ones against the properties an expansion must
       have, and a solve confirming the coefficients reach cdisort.
   * - ``test_input_validation.py``
     - 11
     - **Bad configurations are rejected**, with a message naming the field.
       cdisort's own reaction to an inconsistent state is to write past an
       array or warn on stdout and continue.
   * - ``test_examples.py``
     - 4
     - Runs every script in ``examples/`` and requires it to pass its own
       assertions.

GPU (``tests/cuda/``)
"""""""""""""""""""""

The CUDA path is a separate implementation of the same solver: cdisort is
compiled as ``__host__ __device__`` code and one GPU thread solves one
(wave, column) element. It therefore needs its own correctness checks, and
those need a GPU.

.. list-table::
   :widths: 34 10 56
   :header-rows: 1

   * - File
     - Cases
     - What it checks
   * - ``test_cpu_cuda_agreement.py``
     - 68
     - **CPU and CUDA produce the same fluxes.** The widest configuration
       sweep of the CUDA path: shortwave and longwave, clear-sky and
       scattering, four optical depths and three solar angles at 4 and 8
       streams. It is the only coverage of the **thermal** CUDA path, since
       the routing tests below are shortwave throughout. ``pytest -s`` prints
       the per-configuration agreement metrics.
   * - ``test_fast_flux_routing.py``
     - 22
     - **The fast flux-only kernels agree with the general solver.**
       Specialised CUDA paths for 4 and 8 streams are valid only away from
       conservative scattering, so ``forward`` routes any column within
       :math:`10^{-8}` of :math:`\omega = 1` to the general solver instead.
       These cases pin that routing decision down, including a mixed batch
       that must be routed per column rather than wholesale.

Everything in this directory skips when ``torch.cuda.is_available()`` is
false, so it costs nothing on a CPU-only machine and runs automatically
wherever a GPU is present. Continuous integration has no GPU, so these always
skip there.

Each test module's docstring lists its cases individually -- the optical
depth, single-scattering albedo and source type each one uses, and the
physical regime it probes.

Reading the parameters
""""""""""""""""""""""

Three inputs set the regime a test case probes, and the reference problems
sweep each of them deliberately.

**Optical depth** (:math:`\tau`) is the natural log-attenuation coordinate:
radiation travelling straight through :math:`\tau = 1` is attenuated by
:math:`e^{-1}`. The reference problems pair a thin case with a thick one --
:math:`\tau = 0.03125` against :math:`\tau = 32` in Test Problem 1 -- because
the thin limit checks single scattering while the thick limit checks that
multiple scattering and the diffusion regime are handled correctly.

**Single-scattering albedo** (:math:`\omega`) is the probability that an
extinction event scatters rather than absorbs:

* :math:`\omega = 0.2` -- strong absorption; most radiation is removed;
* :math:`\omega = 0.99` -- weak absorption, the hardest case numerically,
  since energy circulates many times before being lost;
* :math:`\omega = 1` -- conservative scattering, where nothing is absorbed and
  the solution must conserve energy exactly.

**Source type** decides which boundary term drives the problem: ``fbeam`` is a
collimated beam entering at :math:`\mu_0`, ``fisot`` is uniform diffuse
illumination on the top boundary, and the thermal cases are driven by
``planck`` emission from the medium itself. Test Problem 9 uses ``fisot``
alone, scaled to :math:`1/\pi` so the downward flux at the top is exactly 1
and every reported value reads as a fraction of the incident flux.

C and C++ tests
"""""""""""""""

.. list-table::
   :widths: 40 60
   :header-rows: 1

   * - File
     - What it checks
   * - ``tests/test_disort.cpp``
     - The C++ ``DisortOptionsImpl`` / solver API end to end.
   * - ``tests/test_clone.cpp``
     - That cloning a configuration object produces an independent copy.
   * - ``tests/cdisort213/test_cdisort.c``
     - The upstream cdisort test drivers (``disort_test01`` through
       ``disort_test09``) against their published reference output. These are
       the ground truth the Python tests mirror.
   * - ``tests/cdisort213/test_cdisort_09.c``
     - A scaled-up Test Problem 9 driver used as the cdisort baseline in the
       benchmark.

How correctness is established
------------------------------

Three independent kinds of check are used, which is what makes the suite
meaningful rather than merely self-consistent:

**1. Published DISORT reference values.** Everything under
``tests/reference/`` compares computed fluxes and radiances against the
results distributed with DISORT, to a relative tolerance of
:math:`10^{-4}`, which is the precision those tables are quoted to. The C drivers in ``tests/cdisort213/`` check the same
problems at the C level, so a discrepancy can be localised to the backend or to
the bindings.

**2. Analytic solutions.** Where the answer is known in closed form, the
examples check it exactly rather than against a stored number:

* the direct beam must follow Beer-Lambert (Example 1, agreement to machine
  precision);
* an isothermal column over a surface at the same temperature must emit
  :math:`\sigma T^4` at every level (Example 2, relative deviation
  :math:`\sim 10^{-16}`);
* a transparent atmosphere must return the surface albedo exactly
  (Example 3, deviation :math:`\sim 10^{-16}`).

**3. Conservation laws.** These catch errors that a reference comparison at a
few points can miss:

* conservative scattering (:math:`\omega = 1`) over a black surface must lose
  no energy (Example 3, residual :math:`\sim 10^{-9}`, the accuracy of
  DISORT's conservative-scattering special case);
* the column-integrated heating rate must equal the net flux divergence across
  the column (Example 2, exact to rounding);
* a conservative layer over a black surface must lose nothing between the top
  of the atmosphere and the surface (Example 4, residual
  :math:`\sim 10^{-10}`).

Example 4 additionally reproduces the published DISORT benchmark fluxes for
twelve official flux-test cases to **0.0005%**, which is an external check
rather than a self-consistent one.

Correctness is therefore established against published DISORT values,
analytic solutions and conservation laws. pydisort is also compared against
`PythonicDISORT <https://doi.org/10.21105/joss.06442>`_ in
:doc:`benchmarks`, but that comparison exists to measure the
interpreted/compiled performance gap rather than to validate pydisort: the
agreement check there is what makes the runtime ratio meaningful, not a
substitute for the published reference values above.

Continuous integration
----------------------

Every pull request and every push to ``main`` runs, through GitHub Actions
(``.github/workflows/ci.yml``):

* ``pre-commit`` style checks, including ``cpplint`` and ``cppcheck``;
* a CMake build and the full CTest suite on **Ubuntu and macOS**, against
  **Python 3.11 and 3.14**;
* an editable install of the package, so that packaging problems surface in CI
  rather than on PyPI.

Release wheels are built for CPython 3.10-3.14 with ``cibuildwheel``
(``.github/workflows/cd.yml`` and ``release.yml``).

Adding a test
-------------

Python tests are collected by pytest and are also registered with CTest
automatically -- ``tests/CMakeLists.txt`` globs ``tests/**/*.py``, so a new
file needs no build-system change, in ``tests/reference/`` or beside it.

Put it in ``tests/reference/`` if it ports a published DISORT problem, and
build it from the ``solve`` fixture in ``conftest.py`` so that it differs from
its neighbours only in its inputs. Put it beside that directory if it checks
something pydisort adds: batching, an accessor, an input contract.

When adding a test, prefer one of the three kinds above: compare against a
published reference, against an analytic solution, or against a conservation
law. A test that merely records today's output will pass forever without
telling you anything.

See :doc:`contribute` for the contributor workflow.
