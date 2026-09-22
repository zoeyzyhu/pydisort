"""Batching over columns and wavelengths must not change the answer.

This is what pydisort adds on top of cdisort. cdisort solves one atmospheric
column at one wavelength per call; pydisort accepts a
``(nwave, ncol, nlyr, nprop)`` tensor and dispatches over the leading two axes,
which is the whole reason the package exists and the axis it parallelizes.

The axes are independent by construction -- plane-parallel columns do not
exchange radiation, and neither do wavelengths -- so every element of a batch
must reproduce exactly what it would have produced alone. "Exactly" is meant
literally: the same solver runs on the same inputs, so any difference at all
is a batching defect rather than a numerical one.
"""

import numpy as np
import pytest
import torch
from numpy.testing import assert_allclose, assert_equal
from pydisort import Disort, DisortOptions, scattering_moments

NLYR = 4
NSTR = 8
# The output grid is shared by the whole batch, so every column must be at
# least this deep. The thinnest below has a total optical depth of 3.
USER_TAU = [0.0, 0.5, 1.5, 2.5]
USER_MU = [-1.0, -0.3, 0.3, 1.0]

# Four visibly different atmospheres, so that a batch which silently solved one
# column and broadcast it would be caught immediately.
COLUMNS = [
    {"scale": 1.0, "ssalb": 0.2, "umu0": 0.5, "fbeam": np.pi, "albedo": 0.0},
    {"scale": 2.5, "ssalb": 0.9, "umu0": 0.8, "fbeam": 2.0, "albedo": 0.4},
    {"scale": 0.3, "ssalb": 0.99, "umu0": 0.2, "fbeam": 0.0, "albedo": 0.9},
    {"scale": 5.0, "ssalb": 0.5, "umu0": 1.0, "fbeam": 10.0, "albedo": 0.2},
]


def build(ncol, nwave):
    op = DisortOptions().header("batching")
    op.flags("usrtau,usrang,lamber,quiet")
    op.ds().nlyr = NLYR
    op.ds().nstr = NSTR
    op.ds().nmom = NSTR
    op.ds().nphase = NSTR
    op.user_tau(np.array(USER_TAU))
    op.user_mu(np.array(USER_MU))
    op.user_phi(np.array([0.0]))
    op.accur(0.0)
    op.ncol(ncol)
    op.nwave(nwave)
    return Disort(op)


def inputs(specs, nwave=1):
    """Build (prop, bc) for a list of per-column specs."""
    ncol = len(specs)
    prop = torch.zeros((nwave, ncol, NLYR, 2 + NSTR), dtype=torch.float64)
    layer = torch.arange(1, NLYR + 1, dtype=torch.float64)
    for i, spec in enumerate(specs):
        prop[:, i, :, 0] = layer * spec["scale"]
        prop[:, i, :, 1] = spec["ssalb"]
        prop[:, i, :, 2:] = scattering_moments(NSTR, "isotropic")

    def per_wave_column(key):
        values = torch.tensor([s[key] for s in specs], dtype=torch.float64)
        return values.expand(nwave, ncol).contiguous()

    bc = {
        "umu0": torch.tensor([s["umu0"] for s in specs], dtype=torch.float64),
        "phi0": torch.zeros(ncol, dtype=torch.float64),
        "fbeam": per_wave_column("fbeam"),
        "albedo": per_wave_column("albedo"),
        "fisot": torch.zeros((nwave, ncol), dtype=torch.float64),
        "fluor": torch.zeros((nwave, ncol), dtype=torch.float64),
    }
    return prop, bc


def solve(specs, nwave=1):
    ds = build(len(specs), nwave)
    prop, bc = inputs(specs, nwave)
    return ds, ds.forward(prop, **bc)


def test_columns_match_solving_them_one_at_a_time():
    """Each column of a batch equals the same column solved alone."""
    _, batched = solve(COLUMNS)
    assert_equal(batched.shape, (1, len(COLUMNS), len(USER_TAU), 2))

    for i, spec in enumerate(COLUMNS):
        _, alone = solve([spec])
        assert_allclose(
            batched[0, i].numpy(),
            alone[0, 0].numpy(),
            atol=0,
            rtol=0,
            err_msg=f"column {i} changed when solved in a batch",
        )


def test_every_column_is_a_valid_problem():
    """Guard against comparing two identically broken solves.

    DISORT rejects an output depth below the base of the column and returns
    zeros. A batch and a single solve would then agree on nothing at all and
    the comparisons above would still pass, so pin the one value that is known
    in advance: the downward flux at the top is the incident beam flux,
    umu0 * fbeam.
    """
    _, flx = solve(COLUMNS)
    for i, spec in enumerate(COLUMNS):
        expected = spec["umu0"] * spec["fbeam"]
        assert_allclose(
            flx[0, i, 0, 1].item(),
            expected,
            atol=1e-12,
            rtol=1e-9,
            err_msg=f"column {i} did not receive its incident flux",
        )
        assert torch.isfinite(flx[0, i]).all()


def test_columns_are_actually_different():
    """Guard the guard: the columns must not accidentally agree.

    If the specs above ever collapsed to the same atmosphere, the test over
    them would still pass while checking nothing.
    """
    _, batched = solve(COLUMNS)
    first = batched[0, 0]
    for i in range(1, len(COLUMNS)):
        assert not torch.allclose(batched[0, i], first, atol=1e-6)


def test_wavelengths_match_solving_them_one_at_a_time():
    """The spectral axis is independent in the same way the column axis is.

    Every wavelength here carries identical optical properties, which is the
    case a spectral loop hits when only the boundary conditions vary, and it
    is the configuration the benchmarks use.
    """
    nwave = 5
    _, batched = solve(COLUMNS, nwave=nwave)
    assert_equal(batched.shape, (nwave, len(COLUMNS), len(USER_TAU), 2))

    _, single = solve(COLUMNS, nwave=1)
    for w in range(nwave):
        assert_allclose(batched[w].numpy(), single[0].numpy(), atol=0, rtol=0)


def test_wavelengths_with_differing_optical_properties():
    """A spectral batch whose layers differ per wavelength.

    The previous test repeats one atmosphere across the spectral axis, which
    would still pass if `prop` were read only at wave 0. Here each wavelength
    gets its own optical depth, so that shortcut fails.
    """
    nwave, ncol = 3, 1
    ds = build(ncol, nwave)
    prop = torch.zeros((nwave, ncol, NLYR, 2 + NSTR), dtype=torch.float64)
    layer = torch.arange(1, NLYR + 1, dtype=torch.float64)
    spectra = [(0.3, 0.2), (1.0, 0.6), (4.0, 0.95)]
    for w, (scale, ssalb) in enumerate(spectra):
        prop[w, :, :, 0] = layer * scale
        prop[w, :, :, 1] = ssalb
        prop[w, :, :, 2:] = scattering_moments(NSTR, "isotropic")

    bc = {
        "umu0": torch.full((ncol,), 0.5, dtype=torch.float64),
        "phi0": torch.zeros(ncol, dtype=torch.float64),
        "fbeam": torch.full((nwave, ncol), np.pi, dtype=torch.float64),
        "fisot": torch.zeros((nwave, ncol), dtype=torch.float64),
        "fluor": torch.zeros((nwave, ncol), dtype=torch.float64),
        "albedo": torch.zeros((nwave, ncol), dtype=torch.float64),
    }
    batched = ds.forward(prop, **bc)

    for w, (scale, ssalb) in enumerate(spectra):
        _, alone = solve(
            [
                {
                    "scale": scale,
                    "ssalb": ssalb,
                    "umu0": 0.5,
                    "fbeam": np.pi,
                    "albedo": 0.0,
                }
            ]
        )
        assert_allclose(
            batched[w, 0].numpy(), alone[0, 0].numpy(), atol=0, rtol=0
        )

    # More scattering reflects more, so the upward flux at the top must rise
    # with the single-scattering albedo. Reading `prop` only at wave 0 would
    # collapse all three to one value and this ordering would vanish.
    upward = [batched[w, 0, 0, 0].item() for w in range(nwave)]
    assert upward[0] < upward[1] < upward[2], upward


def test_radiances_are_batched_consistently():
    """gather_rad must follow the same rule as forward."""
    ds, _ = solve(COLUMNS)
    rad = ds.gather_rad()
    assert_equal(rad.shape, (1, len(COLUMNS), 1, len(USER_TAU), len(USER_MU)))

    for i, spec in enumerate(COLUMNS):
        alone_ds, _ = solve([spec])
        assert_allclose(
            rad[0, i].numpy(),
            alone_ds.gather_rad()[0, 0].numpy(),
            atol=0,
            rtol=0,
        )


@pytest.mark.parametrize("ncol", [1, 2, 7])
def test_output_shape_follows_the_batch(ncol):
    """Shapes track (nwave, ncol) for any batch size, including ncol = 1."""
    specs = [COLUMNS[i % len(COLUMNS)] for i in range(ncol)]
    ds, flx = solve(specs, nwave=2)
    assert_equal(flx.shape, (2, ncol, len(USER_TAU), 2))
    assert_equal(
        ds.gather_rad().shape, (2, ncol, 1, len(USER_TAU), len(USER_MU))
    )
    assert_equal(ds.gather_flx().shape, (2, ncol, len(USER_TAU), 8))
