# pydisort examples

Runnable, self-checking examples that build up from the simplest possible
DISORT problem to a full research calculation.  Every script is standalone
(copy one file and go), prints its results, and ends with assertions that
check reference values, analytic limits, conservation or internal consistency, so running them is
also a way to verify that your installation is working.

From a repository checkout, install the package and run any example directly:

```bash
pip install pydisort
python examples/example_01_beam_attenuation.py
```

Only `numpy`, `torch` and `pydisort` are required.  `example_04` can
additionally draw a summary figure if `matplotlib` is installed.

## The examples

| Example | What it covers | Validated against |
| --- | --- | --- |
| [`example_01_beam_attenuation.py`](example_01_beam_attenuation.py) | The two-step `DisortOptions` → `Disort.forward` workflow; direct beam attenuation in a purely absorbing atmosphere; reading the flux tensor. | The Beer-Lambert law, to machine precision. |
| [`example_02_thermal_emission.py`](example_02_thermal_emission.py) | Thermal emission with the `planck` flag; **batching over the spectral axis** (8 bands in one call); outgoing longwave radiation and radiative cooling rates for an Earth-like column. | Upward finite-band flux is vertically uniform in an isothermal column; integrated heating is consistent with the flux differences used to define it. |
| [`example_03_aerosol_scattering.py`](example_03_aerosol_scattering.py) | Multiple scattering with a Henyey-Greenstein phase function; radiances at user viewing angles via `gather_rad`; **batching over the column axis** to build a remote-sensing lookup table. | A transparent atmosphere returns the surface albedo exactly; conservative scattering conserves energy. |
| [`example_04_two_stream_validation.py`](example_04_two_stream_validation.py) | **Real-world analysis problem.** Assessing the two-stream DISORT approximation against published fluxes and exploring stream resolution. | The published DISORT benchmark flux values, reproduced to 0.0005% at 16 streams for the selected boundary fluxes. |

## The real-world example

Almost every climate model, weather model and operational retrieval uses a
*two-stream* radiative transfer solver, because two streams are cheap enough to
call millions of times. The price is accuracy, and the only way to know what
that price is, is to compare against a trusted multi-stream reference. DISORT
is that reference, which is why the official DISORT flux-test problems are the
standard yardstick.

`example_04_two_stream_validation.py` demonstrates this real-world analysis
workflow. Projects such as [`py2sess`](https://github.com/happysky19/py2sess)
motivate using DISORT as a reference. Here all calculations use pydisort at
different stream counts; the example does not run an external two-stream solver.

The example runs the workflow end to end:

1. **Reproduce the published DISORT benchmark fluxes.** Twelve official
   flux-test cases (Test 1 isotropic, Test 2 Rayleigh, Test 3
   Henyey-Greenstein) are solved at 16 streams and compared against the
   published values, agreement to **0.0005%**. This validates the
   installation against an external source rather than against itself.
2. **Measure the two-stream error.** The same twelve cases are re-solved with
   `nstr = 2` and `nmom = 2`, supplying moments of orders 1 and 2 plus the
   implicit zeroth moment. The second-order moment can affect delta-M scaling.
   Large relative discrepancies occur for near-zero transmitted fluxes and
   the forward-peaked Henyey-Greenstein cases. These results describe this
   DISORT approximation, not every two-stream closure.
3. **Answer "how many streams do I need?"** by sweeping the stream count from
   2 to 32 and comparing with the published fluxes. The example does not
   time the solves.

Each stream count solves all twelve cases in a single batched `forward` call,
with the cases laid out along the column axis.

Run it with a figure:

```bash
python examples/example_04_two_stream_validation.py --plot two_stream.png
```

## Running them as tests

All four examples are exercised by the test suite:

```bash
pytest tests/test_examples.py -v
```

See [`docs/source/testing.rst`](../docs/source/testing.rst) for the full
description of the test suite.
