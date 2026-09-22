"""DISORT Test Problem 10: user angles versus quadrature angles.

Every other reference problem sets ``usrang``, asking for intensities at
angles the caller chose. Clearing it makes DISORT report at its own Gauss
quadrature angles instead, and that is a genuinely different code path:
intensities are produced directly rather than interpolated, and the output
arrays are sized by ``nstr`` rather than by the length of ``user_mu``.

Upstream runs the medium of Test Problem 9c both ways and requires the two to
agree. That is the check here as well, on fluxes: the angular grid used for
reporting intensities must not disturb the flux integration, which is
performed over the quadrature angles either way.

Fluxes rather than intensities, for two reasons. The published tables for this
problem are flux tables, and ``gather_rad`` currently reads its dimensions
from the pre-allocation state, so it under-reports when ``usrang`` is clear;
that defect is tracked separately and is not what this module is for.

Reference:
    Expected values transcribed from disort_test10() in
    tests/cdisort213/test_cdisort.c.
"""

import math

import pytest
import torch
from conftest import ATOL, RTOL, Layer, Problem
from numpy.testing import assert_allclose, assert_equal

NLYR = 6
NSTR = 4

# The four-stream Gauss quadrature angles, so that requesting them as user
# angles asks for exactly what the quadrature already computes and no
# interpolation error enters the comparison.
QUADRATURE_MU = [-0.788675129, -0.211324871, 0.211324871, 0.788675129]


def _problem(usrang):
    return Problem(
        label=f"Test Problem 10: usrang={usrang}",
        layers=[
            Layer(
                dtau=float(lc),
                ssalb=0.6 + lc * 0.05,
                phase="henyey-greenstein",
                gg=lc / (NLYR + 1),
            )
            for lc in range(1, NLYR + 1)
        ],
        user_tau=[0.0, 2.1, 21.0],
        user_mu=QUADRATURE_MU,
        user_phi=[60.0, 120.0],
        nstr=NSTR,
        umu0=0.5,
        fbeam=math.pi,
        fisot=1.0,
        albedo=0.5,
        temper=[600.0 + 10.0 * i for i in range(NLYR + 1)],
        btemp=700.0,
        ttemp=550.0,
        temis=1.0,
        wavenumber=(999.0, 1000.0),
        usrang=usrang,
    )


@pytest.mark.parametrize("usrang", [True, False])
def test_fluxes_are_independent_of_the_reporting_angles(usrang, solve):
    """Fluxes must be the same whichever angular grid is reported.

    Both configurations integrate over the same quadrature, so any difference
    here would mean the reporting grid had leaked into the flux calculation.
    """
    _, flx = solve(_problem(usrang))
    assert_equal(flx.shape, (1, 1, 3, 2))

    # Solve the same medium the other way and require agreement. Comparing
    # against the sibling run rather than a stored table is the point of this
    # problem: it is a consistency check between two code paths.
    _, other = solve(_problem(not usrang))
    assert_allclose(flx.squeeze(), other.squeeze(), atol=1e-10, rtol=1e-9)


def test_quadrature_path_still_emits_and_reflects(solve):
    """The usrang=False path must produce the same physics, not just a shape.

    A configuration that silently fell back to defaults would still return an
    array of the right size. These are the fingerprints of the inputs: thermal
    emission keeps the deep field bright, and the 0.5 surface albedo sends
    half the flux arriving at the base back up.
    """
    _, flx = solve(_problem(usrang=False))

    upward_at_base = flx[0, 0, -1, 0].item()
    downward_at_base = flx[0, 0, -1, 1].item()

    # Emission from the medium and the 700 K surface keeps both large at
    # tau = 21, where the beam has been extinguished entirely.
    assert upward_at_base > 1.0
    assert downward_at_base > 1.0

    # The surface reflects half of what reaches it, on top of its own
    # emission, so the upward flux must exceed that reflected share.
    assert upward_at_base > 0.5 * downward_at_base
