Automated Tests
===============

pydisort's correctness rests on the fact that DISORT has published reference
results. The test suite checks the Python bindings against those references,
against analytic solutions, and against conservation laws, to detect regressions in the covered configurations. Passing these tests is
not a guarantee for every solver configuration.

Running the tests
-----------------

From a checkout of the repository
`<https://github.com/zoeyzyhu/pydisort>`_:

The Python tests need pydisort and pytest in the active environment:

.. code-block:: bash

  pip install pydisort pytest
  pytest tests/ -v

For development, first follow the source-build procedure in
:doc:`installation` so that tests exercise this checkout rather than an
unrelated PyPI release. Then enable the full CTest suite:

.. code-block:: bash

  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
  cmake --build build
  python -m pip install --no-build-isolation .
  ctest --test-dir build --output-on-failure

To include the examples as tests, configure with ``-DBUILD_EXAMPLES=ON``; they
are also runnable directly through pytest (see below).

What is tested
--------------

The suite groups published-reference and consistency checks under
``tests/reference/``, and API/behavior checks beside it. Failures in either
group can originate in the wrapper, numerical backend or build; the directory
name alone does not diagnose the cause.

Reference conformance (``tests/reference/``)
""""""""""""""""""""""""""""""""""""""""""""

Ports of test problems distributed with DISORT. Problems 1, 2, 3, 6 and 9
compare against stored reference values; problem 10 compares two output-grid
configurations. The modules share the ``solve`` fixture in ``conftest.py``
and also contain problem-specific assertions.

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
       against 16 streams. Exercises a long moment expansion and delta-M
       scaling; activation depends on a nonzero moment at order ``nstr``,
       not simply on ``nmom > nstr``.
   * - ``test_problem_06_lambertian_surface.py``
     - 4
     - **A reflecting surface without scattering.** Isolates Lambertian
       reflection and covers the transparent limit, :math:`\tau = 0`.
       Problems 9c and 10 also exercise reflection.
   * - ``test_problem_09_inhomogeneous.py``
     - 5
     - **Six layers, every one different**, exercising layer-to-layer
       interface matching, also covered by problem 10. Case 9c adds thermal
       emission, a reflecting surface, a per-layer asymmetry parameter and
       three azimuths at once.
   * - ``test_problem_10_user_vs_quadrature.py``
     - 3
     - **Reporting angles do not affect fluxes.** The same medium solved with
       ``usrang`` set and clear, which is a different output path: intensities
       are produced directly rather than interpolated.

The upstream driver invokes fourteen problems; six have Python ports here.
Unported problems are not all unavailable through the wrapper: 4 and 5 add
phase-function cases, 8 and 11 add multilayer/consistency checks, and 12 tests
the absorption-optical-depth shortcut using a Lambertian boundary. These
remain coverage opportunities. Problem 7 includes BRDF cases not exposed by
the Python API, problem 13 uses the special-boundary mode rejected by
``forward``, and problem 14 compares against the separate ``twostr`` solver.

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
     - **Output shapes and flux consistency.** All three accessors report
       the requested level count with ``usrtau`` enabled. ``gather_flx`` is
       reconciled with ``forward``; the ``upward`` ordering check covers
       ``forward`` only. Without ``usrtau``, only ``forward`` and
       ``gather_flx`` shapes are checked, not ``gather_rad``.
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
``planck`` emission from the medium itself. Test Problems 9a and 9b use ``fisot``
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
       ``disort_test14``), printing comparisons with reference output or other
       runs. Numerical discrepancies printed by ``print_test`` do not cause
       a nonzero exit status; CTest success is not a numerical assertion.
   * - ``tests/cdisort213/test_cdisort_09.c``
     - A scaled-up Test Problem 9 driver used as the cdisort baseline in the
       benchmark.

How correctness is established
------------------------------

The suite uses four types of checks:

**1. Published reference values.** Problems 1, 2, 3, 6 and 9 compare fluxes and
radiances against tables distributed with DISORT using ``rtol=1e-4`` and
``atol=1e-6``. The absolute term matters for small or zero reference values;
these tolerances do not mean a relative error below ``1e-4`` everywhere.
Example 4 also compares selected boundary fluxes against published values,
with a reported maximum deviation of about 0.0005% for its 16-stream run.

**2. Analytic limits and conservation.** Example 1 checks direct-beam
Beer-Lambert attenuation. Example 3 checks the transparent-atmosphere surface
reflectance. Examples 3 and 4 check that, for conservative scattering over a
black surface, incoming radiation is reflected upward or reaches the surface.
Those comparisons test specific physical limits, not all operating regimes.

**3. Internal consistency.** Problem 10 compares fluxes with user reporting
angles enabled and disabled, using ``rtol=1e-9`` and ``atol=1e-10``; it does
not compare against a stored table. Example 2 checks that upward flux is
uniform through an isothermal column over a surface at the same temperature,
within its finite spectral bands. It compares levels with the computed
bottom-level flux, not an independent Planck integral or the full
:math:`\sigma T^4`. Its integrated-heating check telescopes the same flux
differences used to define heating: useful for bookkeeping, but not an
independent solver energy-balance test.

**4. Diagnostic drivers and implementation comparisons.** The C reference
driver prints discrepancies but does not propagate them as test failures.
A successful CTest run confirms completion, not that every printed ratio is
within tolerance.
The :doc:`benchmarks` compare complete implementations and require numerical
agreement before timing, complementing the published-reference tests.

Continuous integration
----------------------

Every pull request and every push to ``main`` runs, through GitHub Actions
(``.github/workflows/ci.yml``):

* ``pre-commit`` style checks, including ``cpplint`` and ``cppcheck``;
* a CMake build and the full CTest suite on **Ubuntu and macOS**, against
  **Python 3.11 and 3.14**;
* a regular install using ``pip install --no-build-isolation --no-deps .``
  before CTest runs, exercising the packaging step as well as the C++ build.

Release wheels are built for CPython 3.10-3.14 with ``cibuildwheel``
(``.github/workflows/cd.yml`` and ``release.yml``).

Benchmark thread-control checks, the documentation renderer tests and the
source-tree setup-guidance check are :ref:`separate developer checks
<separate-developer-checks>`, not part of the CI/CTest solver validation.

Adding a test
-------------

Python tests under ``tests/`` are collected by pytest and registered with CTest
when CMake is configured -- ``tests/CMakeLists.txt`` globs ``tests/**/*.py``.
After adding a new file, reconfigure CMake to register it and refresh the
copied Python files in the build tree. No manual test-list edit is needed.

Put it in ``tests/reference/`` if it ports a published DISORT problem, and
build it from the ``solve`` fixture in ``conftest.py`` so that it differs from
its neighbours only in its inputs. Put it beside that directory if it checks
something pydisort adds: batching, an accessor, an input contract.

When adding a test, state what its assertion establishes. Prefer comparisons
against a published reference, an analytic solution or a conservation law.
Snapshots of current output can detect changes, but alone do not establish
that the original result was physically correct.

See :doc:`contribute` for the contributor workflow.
