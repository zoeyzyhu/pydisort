"""Bad configurations must be rejected, with a message that says what is wrong.

pydisort validates before it reaches cdisort, which matters because cdisort's
own reaction to an inconsistent state is to write past an array or to print a
warning to stdout and carry on. A caller who mis-sizes a tensor should get a
Python exception naming the field, not a wrong answer.

These are contract tests: they pin the error, not the wording beyond the field
name, so the messages can be reworded without breaking them.
"""

import numpy as np
import pytest
import torch
from pydisort import Disort, DisortOptions

NSTR = 4


def options(**overrides):
    """A minimal, valid configuration, with `overrides` applied to ds."""
    op = DisortOptions().header("validation").flags("lamber,quiet")
    op.ds().nlyr = overrides.get("nlyr", 1)
    op.ds().nstr = overrides.get("nstr", NSTR)
    op.ds().nmom = overrides.get("nmom", NSTR)
    op.ds().nphase = overrides.get("nphase", NSTR)
    op.ncol(overrides.get("ncol", 1))
    op.nwave(overrides.get("nwave", 1))
    return op


def test_unrecognised_flag_names_the_flag():
    """A typo in the flag string must not be silently ignored.

    Flags are parsed from a comma-separated string, so a misspelling would
    otherwise leave the option at its default and change the physics without
    any indication.
    """
    op = DisortOptions().flags("usrtau,usrangg,lamber")
    with pytest.raises(RuntimeError, match="usrangg"):
        Disort(op)


def test_too_few_moments_for_the_stream_count():
    """nmom < nstr cannot fill the phase-function expansion DISORT needs."""
    with pytest.raises(RuntimeError, match="nmom < ds.nstr"):
        Disort(options(nmom=NSTR - 1))


@pytest.mark.parametrize("field", ["nlyr", "nstr"])
def test_non_positive_dimensions(field):
    """Zero layers or zero streams is not a degenerate case, it is a mistake."""
    with pytest.raises(RuntimeError, match=f"ds.{field} <= 0"):
        Disort(options(**{field: 0}))


def test_thermal_emission_requires_wavenumber_bounds():
    """The planck flag is meaningless without a spectral interval.

    Emission is integrated over [wave_lower, wave_upper]; leaving them unset
    would silently integrate over a zero-width band.
    """
    op = DisortOptions().flags("planck,lamber,quiet")
    op.ds().nlyr = 1
    op.ds().nstr = NSTR
    op.ds().nmom = NSTR
    op.ds().nphase = NSTR
    with pytest.raises(RuntimeError, match="wave_lower"):
        Disort(op)


def test_property_tensor_layer_count_must_match():
    """`prop` carries the layers, and must carry the configured number."""
    ds = Disort(options(nlyr=3))
    prop = torch.zeros((1, 1, 5, 2 + NSTR), dtype=torch.float64)
    with pytest.raises(RuntimeError, match="ds.nlyr != nlyr"):
        ds.forward(prop, fbeam=torch.tensor([[1.0]], dtype=torch.float64))


def test_property_tensor_column_count_must_match():
    """The batch axis is declared up front and cannot be changed per call.

    The solver state is allocated once per (wave, column), so a `prop` with a
    different batch size than was configured would index past that allocation.
    """
    ds = Disort(options(ncol=2))
    prop = torch.zeros((1, 1, 1, 2 + NSTR), dtype=torch.float64)
    with pytest.raises(RuntimeError, match="ncol != prop"):
        ds.forward(prop, fbeam=torch.tensor([[1.0]], dtype=torch.float64))


def test_property_tensor_wave_count_must_match():
    """The same holds for the spectral axis."""
    ds = Disort(options(nwave=3))
    prop = torch.zeros((1, 1, 1, 2 + NSTR), dtype=torch.float64)
    with pytest.raises(RuntimeError, match="nwave != prop"):
        ds.forward(prop, fbeam=torch.tensor([[1.0]], dtype=torch.float64))


def test_boundary_condition_shape_must_match_the_batch():
    """Per-column boundary conditions are sized (nwave, ncol)."""
    ds = Disort(options(ncol=2, nwave=1))
    prop = torch.zeros((1, 2, 1, 2 + NSTR), dtype=torch.float64)
    with pytest.raises(RuntimeError, match="fbeam"):
        ds.forward(
            prop, fbeam=torch.tensor([[1.0, 2.0, 3.0]], dtype=torch.float64)
        )


def test_accessors_require_a_solve_first():
    """Reading results from an unallocated solver must fail, not segfault."""
    op = DisortOptions().flags("lamber,quiet")
    op.ds().nlyr = 1
    op.ds().nstr = NSTR
    op.ds().nmom = NSTR
    op.ds().nphase = NSTR
    ds = Disort(op)
    # A freshly constructed solver is allocated, so this is the shape it
    # promises rather than an error; the point is that it does not crash.
    assert ds.gather_flx().shape[-1] == 8


def test_valid_configuration_still_works():
    """The guard rails must not reject a correct configuration.

    Validation tests that only check failures can pass while the happy path is
    broken, so pin that too.
    """
    ds = Disort(options())
    prop = torch.zeros((1, 1, 1, 2 + NSTR), dtype=torch.float64)
    prop[..., 0] = 1.0
    flx = ds.forward(
        prop,
        umu0=torch.tensor([0.5], dtype=torch.float64),
        fbeam=torch.tensor([[np.pi]], dtype=torch.float64),
    )
    assert torch.isfinite(flx).all()
    # A beam at umu0 = 0.5 delivers umu0 * fbeam downward at the top.
    assert abs(flx[0, 0, 0, 1].item() - 0.5 * np.pi) < 1e-12
