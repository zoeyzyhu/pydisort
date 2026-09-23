"""DISORT Test Problem 3: Henyey-Greenstein scattering.

A single conservatively scattering layer with a strongly forward-peaked phase
function (g = 0.75), with nmom = 32 and nstr = 16. The nonzero moment at
order nstr activates delta-M scaling. This condition can also hold when
nmom == nstr, so this is not exclusive coverage of that code path.

Reference:
    Van de Hulst, H.C., 1980: Multiple Light Scattering, Tables, Formulas and
    Applications, Volumes 1 and 2, Academic Press, New York (VH2, Table 37).
    Expected values transcribed from disort_test03() in
    tests/cdisort213/test_cdisort.c.
"""

import math

import pytest
import torch
from conftest import ATOL, RTOL, Case, Layer, Problem
from numpy.testing import assert_allclose, assert_equal

USER_MU = [-1.0, -0.5, -0.1, 0.1, 0.5, 1.0]
ASYMMETRY = 0.75


def _problem(label, dtau):
    return Problem(
        label=label,
        layers=[
            Layer(
                dtau=dtau,
                ssalb=1.0,
                phase="henyey-greenstein",
                gg=ASYMMETRY,
            )
        ],
        user_tau=[0.0, dtau],
        user_mu=USER_MU,
        nstr=16,
        nmom=32,
        umu0=1.0,
        fbeam=math.pi,
    )


CASES = [
    Case(
        problem=_problem("Test Problem 3a: tau = 1, g = 0.75", 1.0),
        # [upward, downward]; downward is rfldir + rfldn.
        flux=[
            [2.47374e-01, 3.14159e00],
            [0.00000e00, 1.15573e00 + 1.73849e00],
        ],
        radiance=[
            [0.0, 0.0, 0.0, 1.51159e-01, 1.01103e-01, 3.95460e-02],
            [3.05855e00, 2.66648e-01, 2.13750e-01, 0.0, 0.0, 0.0],
        ],
    ),
    Case(
        problem=_problem("Test Problem 3b: tau = 8, g = 0.75", 8.0),
        flux=[
            [1.59096e00, 3.14159e00],
            [0.00000e00, 1.05389e-03 + 1.54958e00],
        ],
        radiance=[
            [0.0, 0.0, 0.0, 3.79740e-01, 5.19598e-01, 4.93302e-01],
            [6.69581e-01, 4.22350e-01, 2.36362e-01, 0.0, 0.0, 0.0],
        ],
    ),
]


@pytest.mark.parametrize("case", CASES, ids=lambda c: c.problem.label[:18])
def test_reference_values(case, solve):
    """Fluxes and radiances must match the published DISORT results."""
    ds, flx = solve(case.problem)

    ntau = len(case.problem.user_tau)
    numu = len(case.problem.user_mu)

    assert_equal(flx.shape, (1, 1, ntau, 2))
    assert_allclose(
        flx.squeeze(0).squeeze(0),
        torch.tensor(case.flux, dtype=torch.float64),
        atol=ATOL,
        rtol=RTOL,
    )

    rad = ds.gather_rad()
    assert_equal(rad.shape, (1, 1, 1, ntau, numu))
    assert_allclose(
        rad.squeeze(),
        torch.tensor(case.radiance, dtype=torch.float64),
        atol=1e-5,
        rtol=RTOL,
    )


def test_conservative_scattering_conserves_energy(solve):
    """With omega = 1 and a black surface, no energy may be absorbed.

    Everything entering the top must leave, either reflected back out or
    transmitted through the base. This holds independently of the reference
    tables, so it catches an error that happened to match them.
    """
    for case in CASES:
        _, flx = solve(case.problem)
        incoming = flx[0, 0, 0, 1]
        outgoing = flx[0, 0, 0, 0] + flx[0, 0, -1, 1]
        assert_allclose((outgoing / incoming).item(), 1.0, atol=1e-6, rtol=0)
