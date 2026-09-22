"""DISORT Test Problem 1: isotropic scattering in a single layer.

A uniform slab over a black surface, swept across optical depth and
single-scattering albedo under two kinds of illumination. It is the simplest
of the reference problems and the one that pins down the basic solver: beam
attenuation, isotropic multiple scattering, and the conservative limit where
no energy may be lost.

Reference:
    Van de Hulst, H.C., 1980: Multiple Light Scattering, Tables, Formulas and
    Applications, Volumes 1 and 2, Academic Press, New York (VH1, Table 12).
    Expected values transcribed from disort_test01() in
    tests/cdisort213/test_cdisort.c.
"""

import math

import pytest
import torch
from conftest import ATOL, RTOL, Case, Layer, Problem
from numpy.testing import assert_allclose, assert_equal

# Three downward and three upward directions, spanning grazing to vertical.
USER_MU = [-1.0, -0.5, -0.1, 0.1, 0.5, 1.0]


CASES = [
    Case(
        problem=Problem(
            label="Test Problem 1a: thin, strongly absorbing, beam",
            layers=[Layer(dtau=0.03125, ssalb=0.2)],
            user_tau=[0.0, 0.03125],
            user_mu=USER_MU,
            nstr=16,
            umu0=0.1,
            fbeam=math.pi / 0.1,
            fisot=0.0,
        ),
        flux=[
            [0.0799451, 3.14159],
            [0, 2.37785],
        ],
        detail=[
            [3.14159, 0, 0.0799451, 25.4067, 2.52725, 0, 0.0272515, 2.5],
            [2.29844, 0.0794111, 0, 18.6531, 1.8555, 0.026421, 0, 1.829],
        ],
        radiance=[
            [0, 0, 0, 0.117771, 0.026417, 0.0134041],
            [0.0133826, 0.0263324, 0.115898, 0, 0, 0],
        ],
    ),
    Case(
        problem=Problem(
            label="Test Problem 1b: thin, conservative scattering, beam",
            layers=[Layer(dtau=0.03125, ssalb=1.0)],
            user_tau=[0.0, 0.03125],
            user_mu=USER_MU,
            nstr=16,
            umu0=0.1,
            fbeam=math.pi / 0.1,
            fisot=0.0,
        ),
        flux=[
            [0.422922, 3.14159],
            [0, 2.71867],
        ],
        detail=[
            [
                3.1416,
                3.1875e-10,
                0.42292,
                7.3777e-13,
                2.6441,
                4.3873e-11,
                0.14407,
                2.5,
            ],
            [
                2.2984,
                0.42023,
                -6.4374e-10,
                5.4939e-13,
                1.9689,
                0.13989,
                -7.5073e-11,
                1.829,
            ],
        ],
        radiance=[
            [0, 0, 0, 0.6229, 0.1398, 0.0709],
            [0.0708, 0.1393, 0.6135, 0, 0, 0],
        ],
    ),
    Case(
        problem=Problem(
            label="Test Problem 1c: thin, near-conservative, isotropic source",
            layers=[Layer(dtau=0.03125, ssalb=0.99)],
            user_tau=[0.0, 0.03125],
            user_mu=USER_MU,
            nstr=16,
            umu0=0.1,
            fbeam=0.0,
            fisot=1.0,
        ),
        flux=[
            [0.0906556, 3.14159],
            [0, 3.04897],
        ],
        detail=[
            [0, 3.1416, 0.090656, 0.066687, 0.53068, 0.5, 0.030679, 0],
            [0, 3.049, -3.2731e-18, 0.058894, 0.46866, 0.46866, -1.18e-17, 0],
        ],
        radiance=[
            [1, 1, 1, 0.1332, 0.03, 0.0152],
            [0.9844, 0.9694, 0.8639, 0, 0, 0],
        ],
    ),
    Case(
        problem=Problem(
            label="Test Problem 1d: thick, strongly absorbing, beam",
            layers=[Layer(dtau=32.0, ssalb=0.2)],
            user_tau=[0.0, 32.0],
            user_mu=USER_MU,
            nstr=16,
            umu0=0.1,
            fbeam=math.pi / 0.1,
            fisot=0.0,
        ),
        flux=[
            [0.259686, 3.14159],
            [0, 0],
        ],
        detail=[
            [3.14159, 0, 0.259686, 25.7766, 2.56404, 0, 0.0640413, 2.5],
            [0, 0, 0, 0, 0, 0, 0, 0],
        ],
        radiance=[
            [0, 0, 0, 0.262972, 0.0906967, 0.0502853],
            [1.2298e-15, 1.30698e-17, 6.8884e-18, 0, 0, 0],
        ],
    ),
    Case(
        problem=Problem(
            label="Test Problem 1e: thick, conservative scattering, beam",
            layers=[Layer(dtau=32.0, ssalb=1.0)],
            user_tau=[0.0, 32.0],
            user_mu=USER_MU,
            nstr=16,
            umu0=0.1,
            fbeam=math.pi / 0.1,
            fisot=0.0,
        ),
        flux=[
            [3.0739, 3.14159],
            [0, 0.0676954],
        ],
        detail=[
            [3.14159, 0, 3.0739, 0, 3.10898, 0, 0.609045, 2.5],
            [0, 0.0676954, 0, 0, 0.00933064, 0.00933064, 0, 0],
        ],
        radiance=[
            [0, 0, 0, 1.93321, 1.02732, 0.797199],
            [0.0271316, 0.0187805, 0.0116385, 0, 0, 0],
        ],
    ),
    Case(
        problem=Problem(
            label="Test Problem 1f: thick, near-conservative, isotropic source",
            layers=[Layer(dtau=32.0, ssalb=0.99)],
            user_tau=[0.0, 32.0],
            user_mu=USER_MU,
            nstr=16,
            umu0=0.1,
            fbeam=0.0,
            fisot=1.0,
        ),
        flux=[
            [2.49618, 3.14159],
            [0, 0.00460048],
        ],
        detail=[
            [0, 3.14159, 2.49618, 0.114239, 0.90909, 0.5, 0.40909, 0],
            [0, 0.00460048, 0, 7.93633e-05, 0.000631548, 0.000631548, 0, 0],
        ],
        radiance=[
            [1, 1, 1, 0.87751, 0.815136, 0.752715],
            [0.0018684, 0.00126492, 0.00077928, 0, 0, 0],
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

    detail = ds.gather_flx()
    assert_equal(detail.shape, (1, 1, ntau, 8))
    assert_allclose(
        detail.squeeze(0).squeeze(0),
        torch.tensor(case.detail, dtype=torch.float64),
        atol=1e-4,
        rtol=1e-4,
    )

    rad = ds.gather_rad()
    assert_equal(rad.shape, (1, 1, 1, ntau, numu))
    assert_allclose(
        rad.squeeze(),
        torch.tensor(case.radiance, dtype=torch.float64),
        atol=1e-4,
        rtol=1e-4,
    )
