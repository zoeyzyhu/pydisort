Example Usage
=============

The `examples/ <https://github.com/zoeyzyhu/pydisort/tree/main/examples>`_
directory contains four complete, runnable calculations that build from the
simplest possible DISORT problem to a research-level analysis.

Every example is standalone (copy one file and run it), prints its results,
and **ends with assertions** that compare against an analytic solution or a
conservation law. Running them is therefore also a way to verify an
installation. They are exercised by the test suite (see :doc:`testing`), so
they cannot silently drift out of date.

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
     - An isothermal column emits exactly :math:`\sigma T^4`; column-integrated
       heating equals the net flux divergence.
   * - ``example_03_aerosol_scattering.py``
     - Multiple scattering with a Henyey-Greenstein phase function; radiances
       via ``gather_rad``; batching over the **column axis** to build a
       remote-sensing lookup table.
     - A transparent atmosphere returns the surface albedo exactly;
       conservative scattering conserves energy.
   * - ``example_04_two_stream_validation.py``
     - **A real-world analysis problem.** Using pydisort as the multi-stream
       reference to measure the error of a fast two-stream solver, over the
       official DISORT flux-test cases.
     - The published DISORT benchmark flux values, reproduced to 0.0005%.

Run any of them directly:

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
eight-band absorber, a warm Lambertian surface, and a US Standard Atmosphere
temperature profile. It produces what a climate model needs from its longwave
scheme: the outgoing longwave radiation and the radiative cooling rate

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
  greenhouse effect            :   130.32 W m-2

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

Example 4: validating a fast two-stream solver
----------------------------------------------

This is the real-world case.

**The problem.** Almost every climate model, weather model and operational
retrieval uses a *two-stream* radiative transfer solver, because two streams
are cheap enough to call millions of times. The price is accuracy, and the only
way to know what that price is, is to compare against a trusted multi-stream
reference. DISORT is that reference, which is why the official DISORT flux-test
problems are the standard yardstick.

This is a real workflow, not a hypothetical one.
`py2sess <https://github.com/happysky19/py2sess>`_ (Le, Li, Natraj & Spurr,
submitted), a differentiable implementation of the two-stream exact
single-scattering method, validates its public level-flux convention against
exactly these DISORT flux-test cases, *"because DISORT is a widely used
multi-stream discrete-ordinate reference solver"*. Their check reports a median
absolute relative difference of 0.36% over the comparison rows, with the large
outliers concentrated where the absolute reference flux is very small or the
phase function is strongly anisotropic.

**The setup.** Twelve official flux-test cases: Test 1 (isotropic scattering,
thin and thick), Test 2 (Rayleigh, moderate and thick) and Test 3
(Henyey-Greenstein, :math:`g = 0.75`, conservative), each a single layer over
a black Lambertian surface. The configurations match the C drivers in
``tests/cdisort213/test_cdisort.c``; cases 1a-1f and 2a-2d are the same
problems as ``tests/reference/test_problem_01_isotropic.py`` and
``tests/reference/test_problem_02_rayleigh.py``. Every
stream count solves all twelve cases in a single batched ``forward`` call, with
the cases laid out along the column axis.

**1. Reproduce the published benchmark values.** Solved at 16 streams, the
stream count the published values were themselves computed at, and compared
against the transcribed DISORT reference fluxes:

.. code-block:: text

   case                    description   quantity     published      pydisort    diff %
     3a        forward-peaked, tau = 1     up TOA      0.247374      0.247374    0.0002
     3a        forward-peaked, tau = 1   down TOA       3.14159       3.14159    0.0001
     3a        forward-peaked, tau = 1   down BOA       2.89422       2.89422   -0.0001
     3b        forward-peaked, tau = 8     up TOA       1.59096       1.59096   -0.0003

  largest deviation from the published values: 0.0005 %

This validates the installation against an external source rather than against
itself.

**2. Measure the two-stream error.** The same twelve cases re-solved with
``nstr = 2``. At two streams only two phase-function moments survive, so a
tabulated or strongly forward-peaked phase function collapses to a single
asymmetry parameter. That truncation *is* the two-stream approximation:

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

The outliers fall in exactly the two places py2sess identifies. First, cases
where the reference flux is nearly zero: 1f and 2c transmit almost nothing, so
a tiny absolute error is a huge relative one. Second, the forward-peaked
Henyey–Greenstein cases 3a and 3b, where two moments cannot represent the phase
function.

.. note::

  These percentiles are not directly comparable to the ones py2sess quotes
  (median 0.36%, 95th 13.3%). That check spans more test cases and includes
  rows that are exact by construction: the imposed top-of-atmosphere boundary
  condition, and the upward flux at a black surface. The scoring here drops
  those rows, so these numbers are the stricter of the two.

**3. How many streams are enough?** Sweeping the stream count gives the
accuracy/cost trade-off directly:

.. code-block:: text

    streams     median       75th       90th       95th        max
          2     3.9560    10.7927    16.4194    62.4908    76.0138
          4     0.2198     1.1629     2.0244     2.3498     2.5648
          8     0.0270     0.1524     0.3145     0.3617     0.9138
         16     0.0001     0.0002     0.0005     0.0005     0.0079
         32     0.0010     0.0035     0.0096     0.0135     0.1139

Four streams already remove most of the two-stream error, and by sixteen the
solution sits on the published values.

The agreement is best at 16 streams and slightly worse at 32, which looks wrong
until you remember what is being measured: the published values were themselves
produced at 16 streams, so the table measures distance from a 16-stream answer,
not distance from the truth. The 32-stream solution is the more accurate one;
it simply differs from the reference by the reference's own discretisation
error.

Run it with a figure:

.. code-block:: bash

  python examples/example_04_two_stream_validation.py --plot two_stream.png

.. note::

  DISORT's input checker prints ``2 streams not recommended`` whenever
  ``nstr = 2``. Since that is precisely what this comparison asks it to do, the
  example redirects the solver's output descriptors around those calls; the
  ``quiet_backend`` helper in the script shows how. Genuine solver failures are
  raised as Python exceptions and are unaffected.
