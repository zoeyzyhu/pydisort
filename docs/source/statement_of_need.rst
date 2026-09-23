Statement of Need
=================

What problem does pydisort solve?
---------------------------------

Radiative transfer describes how electromagnetic radiation travels through and
interacts with a medium such as a planetary atmosphere or ocean water. It
governs how light and thermal radiation are absorbed, scattered and emitted by
gases, aerosols, clouds and surfaces, and it underpins climate modelling,
remote sensing and planetary science.

The Discrete Ordinates Radiative Transfer (DISORT) algorithm [1]_ is one of the
most widely used numerical methods for solving the radiative transfer equation
in plane-parallel media. It discretises the angular domain into a finite set of
propagation directions and solves the resulting coupled system. Of the core
radiative processes, scattering is the most expensive: it couples every
incoming direction to every outgoing one. For realistic atmospheres with many
vertical layers, strong scattering and many wavelengths, radiative transfer
often dominates the total cost of a simulation.

The implementations below illustrate different build and performance
trade-offs. This is context for pydisort's design, not an exhaustive survey
of current Python interfaces:

* The **original Fortran DISORT** [1]_ is the reference implementation, but it
  relies on static memory allocation. The number of layers and streams must be
  fixed at compile time, which is awkward in exploratory or batched workflows.

* **f2py-based wrappers** around the Fortran code improve accessibility, but
  inherit the static-memory constraints and still require users to assemble a
  working Fortran toolchain.

* **PythonicDISORT** [2]_, a pure-Python reimplementation, removes the build
  barrier entirely, at a substantial cost in runtime. On the hardware and
  versions measured in :doc:`benchmarks`, roughly 40x single-threaded. That is
  fine for teaching and for single-column work, but not for high-throughput
  applications.

* **cdisort** [3]_, the C reimplementation used inside libRadtran [4]_, is fast
  and numerically robust, adding dynamic memory allocation, consistent
  double-precision arithmetic and improved intensity corrections, but it is a
  low-level, single-threaded C interface. Python integration and batching
  require an additional interface layer, such as pydisort.

What pydisort provides
----------------------

pydisort builds directly on the well-tested cdisort numerical core rather than
reimplementing DISORT again, and addresses cdisort's accessibility and
scalability limitations. Specifically, it:

#. **eliminates build complexity.** Prebuilt binary wheels are published on
   PyPI for CPython 3.10 through 3.14 on the Linux and macOS targets listed in
   :doc:`installation`, so
   ``pip install pydisort`` needs no compiler, no Fortran toolchain and no
   local build (see :doc:`installation`);
#. **matches compiled performance.** On the workload and hardware in
   :doc:`benchmarks`, single-threaded performance closely matches
   the cdisort baseline;
#. **parallelises the dimensions that matter:** wavelength and atmospheric
   column are naturally separable in plane-parallel radiative transfer, and
   pydisort batches over both, giving roughly an order of magnitude speed-up
   over single-threaded cdisort on a modern multi-core CPU;
#. **exposes an idiomatic Python API** built on keyword arguments and named
   parameters, suitable for interactive use and scripting;
#. **integrates with PyTorch.** Tensors are the primary data structure, so
   radiative transfer drops into tensor-based scientific and machine-learning
   workflows, with a path to GPU execution.

A C++ API is also provided, for embedding pydisort in larger C or C++
simulation frameworks.

Who is it for?
--------------

The target audience is researchers who need accurate radiative transfer
calculations but prefer Python-based, scalable tooling:

* **atmospheric scientists and climate modellers** computing radiative fluxes
  and heating rates, whether offline or inside a general circulation model;
* **remote-sensing researchers** building forward models and retrieval lookup
  tables over large grids of atmospheric states
  (see :doc:`examples`, Example 3);
* **developers of fast approximate solvers**, who need a trusted multi-stream
  reference to quantify the error of a two-stream or otherwise reduced scheme
  (see :doc:`examples`, Example 4);
* **planetary scientists and exoplanet modellers** studying atmospheres across
  the Solar System and beyond;
* **researchers coupling radiative transfer to machine learning**, who need the
  solver to live inside a PyTorch program.

Scope and limitations
---------------------

pydisort inherits DISORT's physical assumptions, and it is worth being explicit
about them:

* the medium is **plane-parallel** and horizontally homogeneous
  within a column. The backend's spherical correction requires geometry inputs
  that are not exposed in Python (see :ref:`python-flag-support`);
* scattering is described by **azimuthally symmetric phase-function moments**;
  polarisation is not treated;
* the underlying numerical engine is cdisort. Agreement with its direct C
  interface is checked for the benchmark configuration; published-reference
  tests use documented absolute and relative tolerances. This does not imply
  machine-precision agreement with every DISORT implementation or configuration;
  see :doc:`testing` and :doc:`benchmarks`.

References
----------

.. [1] Stamnes, K., Tsay, S. C., Wiscombe, W., & Jayaweera, K. (1988).
       Numerically stable algorithm for discrete-ordinate-method radiative
       transfer in multiple scattering and emitting layered media.
       *Applied Optics*, 27(12), 2502-2509.
       https://doi.org/10.1364/AO.27.002502
.. [2] Ho, D. J. (2024). PythonicDISORT: A Python reimplementation of the
       discrete ordinate radiative transfer package DISORT.
       *Journal of Open Source Software*, 9(103), 6442.
       https://doi.org/10.21105/joss.06442
.. [3] Buras, R., Dowling, T., & Emde, C. (2011). New secondary-scattering
       correction in DISORT with increased efficiency for forward scattering.
       *Journal of Quantitative Spectroscopy and Radiative Transfer*, 112(12),
       2028-2034. https://doi.org/10.1016/j.jqsrt.2011.03.019
.. [4] Emde, C., et al. (2016). The libRadtran software package for radiative
       transfer calculations (version 2.0.1). *Geoscientific Model
       Development*, 9(5), 1647-1672. https://doi.org/10.5194/gmd-9-1647-2016
