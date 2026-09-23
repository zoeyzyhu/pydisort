Example Usage
=============

The `examples/ <https://github.com/zoeyzyhu/pydisort/tree/main/examples>`_
directory contains four complete, runnable calculations that build from the
simplest possible DISORT problem to a real-world two-stream validation study.

Every example is standalone (copy one file and run it), prints its results,
and **ends with assertions** against reference values, analytic limits,
conservation laws or internal consistency relations. These checks exercise
specific configurations, not every solver capability. The test suite runs
them (see :doc:`testing`) to catch regressions in the examples.

.. list-table::
   :widths: 22 42 36
   :header-rows: 1

   * - Example
     - What it covers
     - Validated against
   * - ``example_01_beam_attenuation.py``
     - The ``DisortOptions`` → ``Disort.forward`` workflow; beam attenuation in
       a purely absorbing atmosphere; reading the flux tensor.
     - The Beer-Lambert law, to machine precision.
   * - ``example_02_thermal_emission.py``
     - Thermal emission with the ``planck`` flag; batching over the
       **spectral axis**; outgoing longwave radiation and cooling rates.
     - Upward finite-band flux is uniform in an isothermal column;
       integrated heating reproduces the flux differences used to define it.
   * - ``example_03_aerosol_scattering.py``
     - Multiple scattering with a Henyey-Greenstein phase function; radiances
       via ``gather_rad``; batching over the **column axis** to build a
       remote-sensing lookup table.
     - A transparent atmosphere returns the surface albedo exactly;
       conservative scattering conserves energy.
   * - ``example_04_two_stream_validation.py``
     - **Real-world analysis.** Comparing pydisort at 2-32 streams
       against published DISORT fluxes. No external two-stream solver runs.
     - The published DISORT benchmark flux values, reproduced to 0.0005%.

From a repository checkout, run any of them directly (or download one script
and run it from its directory):

.. code-block:: bash

  pip install pydisort
  python examples/example_01_beam_attenuation.py

Only ``numpy``, ``torch`` and ``pydisort`` are required. Example 4 can
additionally draw a summary figure if ``matplotlib`` is installed.

Example 1: beam attenuation
---------------------------

The simplest possible DISORT problem: a purely absorbing, non-emitting medium
illuminated by a collimated beam. The direct beam must follow the Beer-Lambert
law exactly,

.. math::

  F_{\rm dir}(\tau) = F_{\rm beam}\, \mu_0 \, e^{-\tau / \mu_0},

so the answer is known analytically. Three solar zenith angles are solved
simultaneously as three columns. The example also introduces
:meth:`~pydisort.Disort.gather_flx`, which exposes the direct-beam component
separately from the diffuse field.

.. code-block:: text

  Direct beam flux [W m-2] vs. Beer-Lambert
     tau                 mu0=1               mu0=0.5               mu0=0.2
    0.00  1000.0000/1000.0000    500.0000/500.0000     200.0000/200.0000
    0.50   606.5307/606.5307     183.9397/183.9397      16.4170/16.4170
    1.00   367.8794/367.8794      67.6676/67.6676        1.3476/1.3476
    2.00   135.3353/135.3353       9.1578/9.1578         0.0091/0.0091

  maximum absolute error: 0.000e+00 W m-2

Example 2: thermal emission and cooling rates
---------------------------------------------

A complete longwave calculation for an Earth-like atmosphere: an idealised
eight-band absorber, a warm Lambertian surface, and an idealized temperature
profile inspired by the US Standard Atmosphere. A fixed scale height maps
pressure to altitude; this is not the full standard atmosphere or a validated
spectroscopic scheme. It illustrates band-limited outgoing longwave radiation
and the radiative cooling rate

.. math::

  \frac{\partial T}{\partial t} = \frac{g}{c_p} \frac{\partial F_{\rm net}}{\partial p},

where :math:`F_{\rm net}` is the net *upward* flux and levels run from the top
down, so a negative value means the layer is cooling to space.

The point of interest for pydisort users is that **all eight bands are solved
in a single call** by placing them on the leading ``nwave`` axis.

.. code-block:: text

  Per-band outgoing longwave radiation
       band [cm-1]  kappa [m2/kg]  tau_total   OLR [W/m2]  label
            10-400        2.0e-03       6.87        50.09  H2O rotational
           600-800        3.0e-03      15.49        27.43  CO2 15 um
          800-1000        2.0e-05       0.10        58.06  window
             total                                 259.79

  OLR                          :   259.79 W m-2
  surface emission (sigma T^4) :   390.11 W m-2
  full-spectrum surface minus band OLR:   130.32 W m-2

The surface value above integrates the entire spectrum, whereas OLR includes
only the example's 10-3000 cm\ :sup:`-1` bands. Their difference includes
omitted-band emission as well as atmospheric effects; it is not a like-for-like
broadband greenhouse diagnostic.

The isothermal check compares every level's upward flux with the computed
bottom-level flux. It verifies vertical uniformity, not absolute agreement
with :math:`\sigma T^4`. Integrating the derived heating rate telescopes the
same flux differences, so that check verifies bookkeeping rather than an
independent physical energy budget.

Example 3: aerosol scattering and a retrieval lookup table
----------------------------------------------------------

Scattering is what DISORT exists for, and the expensive part of any radiative
transfer calculation. This example builds the forward model behind a satellite
aerosol retrieval: top-of-atmosphere reflectance as a function of aerosol
optical depth and viewing geometry, for a Henyey-Greenstein aerosol over a
Lambertian surface.

A retrieval needs that forward model on a whole grid of optical depths, so
**all eight optical depths are solved in one call** on the column axis. The
beam flux is normalised as :math:`F_{\rm beam} = \pi / \mu_0`, which turns the
reported radiance into a bidirectional reflectance. A bare Lambertian surface
of albedo :math:`A` then reads back exactly :math:`A`, which is the first
validation check.

.. code-block:: text

  relative azimuth phi = 0 deg
     AOD    mu=0.3    mu=0.5    mu=0.7      mu=1
    0.00    0.1000    0.1000    0.1000    0.1000
    0.20    0.3392    0.1934    0.1385    0.1053
    0.80    0.7158    0.4297    0.2671    0.1370
    3.20    0.9028    0.6506    0.4642    0.2503

.. _real-world-example:

Example 4: a real-world two-stream validation study
---------------------------------------------------

This example demonstrates the reference-comparison workflow used to assess
radiative-transfer approximations. It compares pydisort with published flux
values and varies pydisort's own stream count; it does not run a separate
two-stream implementation. Projects such as
`py2sess <https://github.com/happysky19/py2sess>`_ provide motivation for such
comparisons, but this example does not measure their accuracy or performance.

**The setup.** Twelve official flux-test cases: Test 1 (isotropic scattering,
thin and thick), Test 2 (Rayleigh, moderate and thick) and Test 3
(Henyey-Greenstein, :math:`g = 0.75`, conservative), each a single layer over
a black Lambertian surface. The configurations match the C drivers in
``tests/cdisort213/test_cdisort.c``; cases 1a-1f and 2a-2d are the same
problems as ``tests/reference/test_problem_01_isotropic.py`` and
``tests/reference/test_problem_02_rayleigh.py``. Every
stream count solves all twelve cases in a single batched ``forward`` call, with
the cases laid out along the column axis.

**1. Reproduce the published benchmark values.** The example uses a
16-stream run to compare against the transcribed DISORT reference fluxes:

.. code-block:: text

   case                    description   quantity     published      pydisort    diff %
     3a        forward-peaked, tau = 1     up TOA      0.247374      0.247374    0.0002
     3a        forward-peaked, tau = 1   down TOA       3.14159       3.14159    0.0001
     3a        forward-peaked, tau = 1   down BOA       2.89422       2.89422   -0.0001
     3b        forward-peaked, tau = 8     up TOA       1.59096       1.59096   -0.0003

  largest deviation from the published values: 0.0005 %

This validates the installation against an external source rather than against
itself.

**2. Measure the two-stream DISORT discrepancy.** The same twelve cases are
re-solved with ``nstr = 2``. This changes the angular quadrature, and this
script also sets ``nmom = nstr``. It supplies moments of orders 1 through
``nstr`` in addition to the implicit zeroth moment; the coefficient at order
``nstr`` can affect delta-M scaling. This is not just retaining a single
asymmetry parameter, nor is it a test of every two-stream closure:

.. code-block:: text

   case                    description       up TOA    down BOA     net TOA     net BOA
     1a          thin, absorbing, beam        3.03        0.11       -0.08        0.11
     1e      thick, conservative, beam        0.34      -15.62      -15.62      -15.62
     1f    thick, isotropic top source        2.97      -62.49      -11.50      -62.49
     2c     Rayleigh, thick, absorbing        1.00      -76.01       -0.33      -76.01
     3a        forward-peaked, tau = 1       17.22       -1.47       -1.47       -1.47
     3b        forward-peaked, tau = 8       10.59      -10.86      -10.86      -10.86

  over 46 scored rows, |relative difference| in percent:
    median 3.96   75th 10.79   90th 16.42   95th 62.49   max 76.01

Large relative discrepancies occur for near-zero transmitted fluxes (1f and
2c), where the denominator magnifies a small absolute difference, and for the
forward-peaked Henyey-Greenstein cases (3a and 3b), where angular resolution
matters. Interpret relative errors alongside absolute fluxes.

The percentiles score upward TOA, downward BOA and net boundary fluxes,
excluding zero references. They omit imposed downward TOA and black-surface
upward fluxes, which are exact by construction. Results from another solver
or a different selection of cases/rows are not directly comparable; this
selection alone does not establish a "stricter" validation.

**3. How sensitive are these results to stream count?** The sweep reports
discrepancies from the published fluxes; it does not measure runtime:

.. code-block:: text

    streams     median       75th       90th       95th        max
          2     3.9560    10.7927    16.4194    62.4908    76.0138
          4     0.2198     1.1629     2.0244     2.3498     2.5648
          8     0.0270     0.1524     0.3145     0.3617     0.9138
         16     0.0001     0.0002     0.0005     0.0005     0.0079
         32     0.0010     0.0035     0.0096     0.0135     0.1139

Four streams already remove most of the two-stream error, and by sixteen the
solution sits on the published values.

Agreement with these tabulated values is best at 16 streams and slightly
worse at 32. The table measures distance from finite-precision reference
values, not from an exact solution. This result alone does not prove that
32 streams is more accurate, or that discrepancies come solely from the
reference's discretization. Assess convergence for the quantities and
configurations relevant to your application.

Run it with a figure:

.. code-block:: bash

  python examples/example_04_two_stream_validation.py --plot two_stream.png

.. note::

  DISORT's input checker prints ``2 streams not recommended`` whenever
  ``nstr = 2``. Since that is precisely what this comparison asks it to do, the
  example redirects the solver's output descriptors around those calls; the
  ``quiet_backend`` helper in the script shows how. Genuine solver failures are
  raised as Python exceptions and are unaffected.
