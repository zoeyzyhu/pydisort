# pydisort examples

Runnable, self-checking examples that build up from the simplest possible
DISORT problem to a full research calculation.  Every script is standalone
(copy one file and go), prints its results, and ends with assertions that
compare against an analytic solution or a conservation law, so running them is
also a way to verify that your installation is working.

Install the package and run any of them directly:

```bash
pip install pydisort
python examples/example_01_beam_attenuation.py
```

Only `numpy`, `torch` and `pydisort` are required.  `example_04` can
additionally draw a summary figure if `matplotlib` is installed.

## The examples

| Example | What it covers | Validated against |
| --- | --- | --- |
| [`example_01_beam_attenuation.py`](example_01_beam_attenuation.py) | The two-step `DisortOptions` → `Disort.forward` workflow; direct beam attenuation in a purely absorbing atmosphere; reading the flux tensor. | The Beer–Lambert law, to machine precision. |
| [`example_02_thermal_emission.py`](example_02_thermal_emission.py) | Thermal emission with the `planck` flag; **batching over the spectral axis** (8 bands in one call); outgoing longwave radiation and radiative cooling rates for an Earth-like column. | An isothermal column emits exactly σT⁴; the column-integrated heating rate equals the net flux divergence. |
| [`example_03_aerosol_scattering.py`](example_03_aerosol_scattering.py) | Multiple scattering with a Henyey–Greenstein phase function; radiances at user viewing angles via `gather_rad`; **batching over the column axis** to build a remote-sensing lookup table. | A transparent atmosphere returns the surface albedo exactly; conservative scattering conserves energy. |
| [`example_04_two_stream_validation.py`](example_04_two_stream_validation.py) | **Real-world analysis problem.** Using pydisort as the multi-stream reference to measure the error of a fast two-stream solver, over the official DISORT flux-test cases. | The published DISORT benchmark flux values, reproduced to 0.0005%. |

## The real-world example

Almost every climate model, weather model and operational retrieval uses a
*two-stream* radiative transfer solver, because two streams are cheap enough to
call millions of times. The price is accuracy, and the only way to know what
that price is, is to compare against a trusted multi-stream reference. DISORT
is that reference, which is why the official DISORT flux-test problems are the
standard yardstick.

`example_04_two_stream_validation.py` carries out that comparison. It is a real
workflow, not a hypothetical one: [`py2sess`](https://github.com/happysky19/py2sess)
(Le, Li, Natraj & Spurr, submitted), a differentiable implementation of the
two-stream exact single-scattering method, validates its public level-flux
convention against exactly these DISORT flux-test cases, *"because DISORT is a
widely used multi-stream discrete-ordinate reference solver"*.

The example runs the workflow end to end:

1. **Reproduce the published DISORT benchmark fluxes.** Twelve official
   flux-test cases (Test 1 isotropic, Test 2 Rayleigh, Test 3
   Henyey–Greenstein) are solved at 16 streams and compared against the
   published values, agreement to **0.0005%**. This validates the
   installation against an external source rather than against itself.
2. **Measure the two-stream error.** The same twelve cases are re-solved with
   `nstr = 2`, which reduces the phase function to a single asymmetry
   parameter. That truncation *is* the two-stream approximation. The errors
   concentrate exactly where py2sess reports theirs: on near-zero fluxes,
   where a tiny absolute error is a huge relative one, and on the
   forward-peaked Henyey–Greenstein cases, where two moments cannot represent
   the phase function.
3. **Answer "how many streams do I need?"** by sweeping the stream count from
   2 to 32, the accuracy/cost trade-off for a given problem.

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
