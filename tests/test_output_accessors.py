"""The three ways of reading a solve must agree with each other.

``forward`` returns upward and downward flux at each output depth,
``gather_flx`` the eight-component cdisort breakdown, and ``gather_rad`` the
intensities. They read the same solve through different paths, so they have to
be consistent: same level count, same fluxes, same ordering.

This module checks requested-depth shapes for all three accessors, flux
reconciliation with gather_flx, and upward ordering for forward only. The
no-usrtau check covers forward and gather_flx shapes, not gather_rad.

That consistency is not automatic. ``gather_flx`` used to size its output
``nlyr + 1``, which is the level count only when ``usrtau`` is off, and read
past the end of cdisort's ``out->rad`` whenever a caller asked for fewer
output depths than there are layer boundaries. The shape checks below are what
pins that down.
"""

import numpy as np
import pytest
import torch
from numpy.testing import assert_allclose, assert_equal
from pydisort import Disort, DisortOptions, scattering_moments

NLYR = 6
NSTR = 8
USER_MU = [-1.0, -0.4, 0.4, 1.0]

# gather_flx column order, from the cdisort disort_radiant struct.
RFLDIR, RFLDN, FLUP = 0, 1, 2


def build(user_tau, upward=0, usrtau=True, ncol=1):
    flags = "usrang,lamber,quiet"
    if usrtau:
        flags = "usrtau," + flags
    op = DisortOptions().header("accessors").flags(flags)
    op.ds().nlyr = NLYR
    op.ds().nstr = NSTR
    op.ds().nmom = NSTR
    op.ds().nphase = NSTR
    if usrtau:
        op.user_tau(np.array(user_tau))
    op.user_mu(np.array(USER_MU))
    op.user_phi(np.array([0.0]))
    op.accur(0.0)
    op.ncol(ncol)
    op.nwave(1)
    op.upward(upward)

    ds = Disort(op)
    layer = torch.arange(1, NLYR + 1, dtype=torch.float64)
    prop = torch.zeros((1, ncol, NLYR, 2 + NSTR), dtype=torch.float64)
    dtau = layer / NLYR * 6.0
    ssalb = 0.6 + layer * 0.05
    if upward:
        dtau, ssalb = dtau.flip(0), ssalb.flip(0)
    prop[:, :, :, 0] = dtau
    prop[:, :, :, 1] = ssalb
    prop[:, :, :, 2:] = scattering_moments(NSTR, "isotropic")

    bc = {
        "umu0": torch.full((ncol,), 0.5, dtype=torch.float64),
        "phi0": torch.zeros(ncol, dtype=torch.float64),
        "fbeam": torch.full((1, ncol), np.pi, dtype=torch.float64),
        "fisot": torch.zeros((1, ncol), dtype=torch.float64),
        "fluor": torch.zeros((1, ncol), dtype=torch.float64),
        "albedo": torch.full((1, ncol), 0.3, dtype=torch.float64),
    }
    return ds, ds.forward(prop, **bc)


@pytest.mark.parametrize(
    "user_tau",
    [
        [0.0, 21.0],  # fewer levels than layer boundaries
        [0.0, 1.05, 2.1, 6.0, 21.0],  # still fewer: 5 against nlyr + 1 = 7
        [0.0, 1.0, 3.0, 6.0, 10.0, 15.0, 21.0],  # exactly nlyr + 1
        [0.0, 0.5, 1.0, 2.0, 4.0, 8.0, 15.0, 21.0],  # more than nlyr + 1
    ],
    ids=["two", "fewer", "equal", "more"],
)
def test_level_count_follows_user_tau(user_tau):
    """Every accessor reports exactly the requested output depths.

    The interesting cases are the ones where ntau differs from nlyr + 1, in
    either direction. A level count derived from nlyr rather than from the
    request would pass the `equal` case and fail all three others.
    """
    ds, flx = build(user_tau)
    ntau = len(user_tau)

    assert_equal(flx.shape, (1, 1, ntau, 2))
    assert_equal(ds.gather_flx().shape, (1, 1, ntau, 8))
    assert_equal(ds.gather_rad().shape, (1, 1, 1, ntau, len(USER_MU)))


def test_gather_flx_reconciles_with_forward():
    """forward's two columns are gather_flx's components, recombined.

    forward reports [upward, downward] with downward being direct plus
    diffuse; gather_flx keeps rfldir, rfldn and flup separate. The two views
    come from one solve, so they must agree exactly, not merely closely.
    """
    user_tau = [0.0, 1.05, 2.1, 6.0, 21.0]
    ds, flx = build(user_tau)
    detail = ds.gather_flx()

    assert_allclose(
        detail[0, 0, :, FLUP].numpy(), flx[0, 0, :, 0].numpy(), atol=0, rtol=0
    )
    assert_allclose(
        (detail[0, 0, :, RFLDIR] + detail[0, 0, :, RFLDN]).numpy(),
        flx[0, 0, :, 1].numpy(),
        atol=0,
        rtol=0,
    )


def test_gather_flx_has_no_uninitialised_tail():
    """No reported level may contain heap garbage.

    When gather_flx was sized from nlyr it returned whatever lay past the end
    of the allocation, which showed up as values like -4e+108. Bounding every
    component by the incident flux catches that without depending on the
    particular garbage a given allocator leaves behind.
    """
    ds, _ = build([0.0, 1.05, 2.1, 6.0, 21.0])
    detail = ds.gather_flx()

    assert torch.isfinite(detail).all()
    # Nothing in this problem can exceed the incident beam flux of pi.
    assert detail[0, 0, :, :3].abs().max().item() <= np.pi + 1e-9


def test_without_usrtau_levels_are_the_layer_boundaries():
    """Clearing usrtau reports at every layer boundary instead.

    This is the configuration where ntau does equal nlyr + 1, and where the
    old gather_flx happened to be correct. Keeping it alongside the cases
    above documents why the bug stayed hidden.
    """
    ds, flx = build(user_tau=None, usrtau=False)
    assert_equal(flx.shape, (1, 1, NLYR + 1, 2))
    assert_equal(ds.gather_flx().shape, (1, 1, NLYR + 1, 8))


def test_upward_reverses_the_profile_and_nothing_else():
    """The `upward` option flips the vertical ordering on input and output.

    With upward set, layers are supplied bottom-first and results come back
    bottom-first. The same physical atmosphere must therefore give the same
    numbers in reverse, which is a stronger check than either run alone.
    """
    user_tau = [0.0, 1.05, 2.1, 6.0, 21.0]
    _, downward = build(user_tau, upward=0)
    _, upward = build(user_tau, upward=1)

    assert_allclose(
        upward.squeeze().flip(0).numpy(),
        downward.squeeze().numpy(),
        atol=0,
        rtol=0,
    )


def test_gather_rad_is_rejected_in_flux_only_mode():
    """onlyfl means no intensities were computed, so asking must fail loudly.

    Returning an empty or stale array here would be worse than an error.
    """
    op = DisortOptions().header("flux only").flags("onlyfl,lamber,quiet")
    op.ds().nlyr = 1
    op.ds().nstr = 4
    op.ds().nmom = 4
    op.ds().nphase = 4
    op.ncol(1)
    op.nwave(1)
    ds = Disort(op)

    prop = torch.zeros((1, 1, 1, 6), dtype=torch.float64)
    prop[..., 0] = 1.0
    ds.forward(prop, fbeam=torch.tensor([[np.pi]], dtype=torch.float64))

    with pytest.raises(RuntimeError, match="onlyfl"):
        ds.gather_rad()
