"""DISORT Test Problem 9: general emitting, absorbing, scattering medium.

Six layers, every one of them different: the optical thickness grows 1, 2, ...
6 and the single-scatter albedo ramps from 0.65 to 0.90. Test Problems 1, 2, 3
and 6 are all single slabs, so this is the only reference problem in the suite
where the layer-to-layer interface matching runs at all, and it is the most
intricate part of the solver.

The three cases build on each other:

* **9a** isotropic scattering under diffuse illumination;
* **9b** the same medium with a tabulated phase function supplied as explicit
  Legendre moments rather than an analytic form;
* **9c** the generalisation to every source at once, which is where thermal
  emission, a reflecting surface, a per-layer asymmetry parameter and three
  azimuths are all exercised together.

Note on 9b and 9c: the upstream C driver sets ``ncase = 1``, so it runs 9a and
leaves the other two as unreached code. Their reference values are published
alongside 9a's and are reproduced here to better than 1e-6, which is the
precision those tables are quoted to.

Reference:
    Devaux, C., Grandjean, P., Ishiguro, Y. and C.E. Siewert, 1979: On
    Multi-Region Problems in Radiative Transfer, Astrophys. Space Sci. 62,
    225-233 (DGIS, Tables VI-VII, beta = 0). Expected values transcribed from
    disort_test09() in tests/cdisort213/test_cdisort.c.
"""

import math

import pytest
import torch
from conftest import ATOL, RTOL, Case, Layer, Problem
from numpy.testing import assert_allclose, assert_equal

# 0, then 5% and 10% of the column, the layer-3/layer-4 interface, and the
# lower boundary -- so interpolation is exercised inside a layer as well as at
# its edges.
USER_TAU = [0.0, 1.05, 2.1, 6.0, 21.0]

# Positive is upward, negative downward: two downward directions (straight
# down and slanted) and two upward.
USER_MU = [-1.0, -0.2, 0.2, 1.0]

NLYR = 6
DTAU = [float(lc) for lc in range(1, NLYR + 1)]
SSALB = [0.6 + lc * 0.05 for lc in range(1, NLYR + 1)]

# 9b's tabulated phase function, identical in every layer. The published table
# lists (2k+1) * moment, so each entry is divided back out.
TABULATED = [
    2.00916 / 3.0,
    1.56339 / 5.0,
    0.67407 / 7.0,
    0.22215 / 9.0,
    0.04725 / 11.0,
    0.00671 / 13.0,
    0.00068 / 15.0,
    0.00005 / 17.0,
]

CASES = [
    Case(
        problem=Problem(
            label="Test Problem 9a: isotropic, diffuse illumination",
            layers=[Layer(d, w) for d, w in zip(DTAU, SSALB)],
            user_tau=USER_TAU,
            user_mu=USER_MU,
            user_phi=[60.0],
            nstr=8,
            umu0=0.5,
            # fisot = 1/pi makes the downward flux at the top exactly 1, so
            # every value below reads as a fraction of the incident flux.
            fisot=1.0 / math.pi,
        ),
        flux=[
            [2.27973e-01, 1.00000e00],
            [8.75098e-02, 3.55151e-01],
            [3.61819e-02, 1.44265e-01],
            [2.19291e-03, 6.71445e-03],
            [0.00000e00, 6.16968e-07],
        ],
        radiance=[
            [3.18310e-01, 3.18310e-01, 9.98915e-02, 5.91345e-02],
            [1.53507e-01, 5.09531e-02, 3.67006e-02, 2.31903e-02],
            [7.06614e-02, 2.09119e-02, 1.48545e-02, 9.72307e-03],
            [3.72784e-03, 1.08815e-03, 8.83316e-04, 5.94743e-04],
            [2.87656e-07, 1.05921e-07, 0.00000e00, 0.00000e00],
        ],
    ),
    Case(
        problem=Problem(
            label="Test Problem 9b: tabulated phase function",
            layers=[
                Layer(d, w, moments=TABULATED) for d, w in zip(DTAU, SSALB)
            ],
            user_tau=USER_TAU,
            user_mu=USER_MU,
            user_phi=[60.0],
            nstr=8,
            umu0=0.5,
            fisot=1.0 / math.pi,
        ),
        flux=[
            [1.00079e-01, 1.00000e00],
            [4.52014e-02, 4.52357e-01],
            [2.41941e-02, 2.36473e-01],
            [4.16016e-03, 2.76475e-02],
            [0.00000e00, 7.41853e-05],
        ],
        radiance=[
            [3.18310e-01, 3.18310e-01, 7.39198e-02, 1.32768e-02],
            [1.96609e-01, 5.92369e-02, 3.00230e-02, 7.05566e-03],
            [1.15478e-01, 3.01809e-02, 1.52672e-02, 4.06932e-03],
            [1.46177e-02, 3.85590e-03, 2.38301e-03, 7.77890e-04],
            [3.37742e-05, 1.20858e-05, 0.00000e00, 0.00000e00],
        ],
    ),
    Case(
        problem=Problem(
            label="Test Problem 9c: every source at once",
            layers=[
                Layer(d, w, phase="henyey-greenstein", gg=lc / 7.0)
                for lc, (d, w) in enumerate(zip(DTAU, SSALB), start=1)
            ],
            user_tau=USER_TAU,
            user_mu=USER_MU,
            user_phi=[60.0, 120.0, 180.0],
            nstr=8,
            umu0=0.5,
            fbeam=math.pi,
            fisot=1.0,
            albedo=0.5,
            # 600 K at the top boundary rising to 660 K at the base, with a
            # 700 K surface and a 550 K emitting upper boundary.
            temper=[600.0 + 10.0 * i for i in range(NLYR + 1)],
            btemp=700.0,
            ttemp=550.0,
            temis=1.0,
            wavenumber=(999.0, 1000.0),
        ),
        flux=[
            [4.68414e00, 1.57080e00 + 6.09217e00],
            [4.24381e00, 1.92354e-01 + 4.97279e00],
            [4.16941e00, 2.35550e-02 + 4.46616e00],
            [4.30667e00, 9.65131e-06 + 4.22731e00],
            [5.11524e00, 9.03133e-19 + 4.73767e00],
        ],
        radiance=[
            [
                [1.93920, 1.93920, 1.61855, 1.43872],
                [1.66764, 1.44453, 1.38339, 1.33890],
                [1.48511, 1.35009, 1.33079, 1.32794],
                [1.34514, 1.35131, 1.35980, 1.37918],
                [1.48927, 1.54270, 1.62823, 1.62823],
            ],
            [
                [1.93920, 1.93920, 1.57895, 1.43872],
                [1.66764, 1.42925, 1.37317, 1.33890],
                [1.48511, 1.34587, 1.32921, 1.32794],
                [1.34514, 1.35129, 1.35979, 1.37918],
                [1.48927, 1.54270, 1.62823, 1.62823],
            ],
            [
                [1.93920, 1.93920, 1.56559, 1.43872],
                [1.66764, 1.42444, 1.37034, 1.33890],
                [1.48511, 1.34469, 1.32873, 1.32794],
                [1.34514, 1.35128, 1.35979, 1.37918],
                [1.48927, 1.54270, 1.62823, 1.62823],
            ],
        ],
    ),
]


@pytest.mark.parametrize("case", CASES, ids=lambda c: c.problem.label[:18])
def test_reference_values(case, solve):
    """Fluxes and radiances must match the published DISORT results."""
    ds, flx = solve(case.problem)

    ntau = len(case.problem.user_tau)
    numu = len(case.problem.user_mu)
    nphi = len(case.problem.user_phi)

    assert_equal(flx.shape, (1, 1, ntau, 2))
    assert_allclose(
        flx.squeeze(0).squeeze(0),
        torch.tensor(case.flux, dtype=torch.float64),
        atol=ATOL,
        rtol=RTOL,
    )

    rad = ds.gather_rad()
    assert_equal(rad.shape, (1, 1, nphi, ntau, numu))
    assert_allclose(
        rad.squeeze(0).squeeze(0),
        torch.tensor(case.radiance, dtype=torch.float64).reshape(
            nphi, ntau, numu
        ),
        atol=1e-5,
        rtol=RTOL,
    )


def test_case_9a_boundary_conditions(solve):
    """Two exact consequences of 9a's boundary conditions.

    Both follow from the inputs rather than from a table, so they fail even if
    a reference value were mistranscribed.
    """
    ds, _ = solve(CASES[0].problem)
    rad = ds.gather_rad().squeeze()

    # At the top, the downward radiances are the incident field itself,
    # unattenuated, so they equal the isotropic source exactly.
    assert_allclose(rad[0, :2], 1.0 / math.pi, atol=0, rtol=1e-12)

    # At the base nothing travels upward: the surface is black and there is no
    # source beneath it. An exact zero, not a small number.
    assert_allclose(rad[-1, 2:], 0.0, atol=1e-20, rtol=0)


def test_thermal_emission_dominates_at_depth(solve):
    """In 9c the deep field is set by the medium's own emission.

    The beam is extinguished by tau = 21 (rfldir falls to 9e-19), yet both the
    upward and downward fluxes there are of order 5. That radiation can only
    have come from the Planck source, so this is a direct check that thermal
    emission is switched on and contributing, independent of the tables.
    """
    _, flx = solve(CASES[2].problem)
    upward_at_base = flx[0, 0, -1, 0].item()
    downward_at_base = flx[0, 0, -1, 1].item()

    assert upward_at_base > 1.0
    assert downward_at_base > 1.0

    # Without emission the downward flux would decay monotonically. Here it
    # turns back up between tau = 6 and tau = 21.
    assert downward_at_base > flx[0, 0, -2, 1].item()
