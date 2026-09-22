"""DISORT Test Problem 2: Rayleigh scattering, beam source.

The same single-layer geometry as Test Problem 1, but with a Rayleigh phase
function instead of an isotropic one. Rayleigh scattering has a non-zero
second Legendre moment, so this is the first problem where the angular
redistribution matters and the phase-function expansion is exercised.

Reference:
    Coulson, K.L., J.V. Dave and Z. Sekera, 1960: Tables Related to Radiation
    Emerging from a Planetary Atmosphere with Rayleigh Scattering, Univ. of
    Calif. Press, Berkeley (SW, Table 1). Expected values transcribed from
    disort_test02() in tests/cdisort213/test_cdisort.c.
"""

import math

import pytest
import torch
from conftest import ATOL, RTOL, Case, Layer, Problem
from numpy.testing import assert_allclose, assert_equal

# The Gauss quadrature angles of the 16-stream solution, so that no
# interpolation error enters the comparison.
USER_MU = [-0.981986, -0.538263, -0.018014, 0.018014, 0.538263, 0.981986]


CASES = [
    Case(
        problem=Problem(
            label="Test Problem 2a: moderate depth, partial scattering",
            layers=[Layer(dtau=0.2, ssalb=0.5, phase="rayleigh")],
            user_tau=[0.0, 0.2],
            user_mu=USER_MU,
            nstr=16,
            umu0=0.080442,
            fbeam=math.pi,
        ),
        flux=[
            [0.0535063, 0.252716],
            [0, 0.0652102],
        ],
        detail=[
            [
                0.25272,
                0,
                0.053506,
                1.6657,
                0.2651,
                -7.7061e-18,
                0.015104,
                0.25,
            ],
            [
                0.021031,
                0.044179,
                -1.6106e-18,
                0.18985,
                0.030215,
                0.0094101,
                -2.2606e-19,
                0.020805,
            ],
        ],
        radiance=[
            [0, 0, 0, 0.1618, 0.0212, 0.0079],
            [0.0077, 0.0201, 0.0258, 0, 0, 0],
        ],
    ),
    Case(
        problem=Problem(
            label="Test Problem 2b: moderate depth, conservative scattering",
            layers=[Layer(dtau=0.2, ssalb=1.0, phase="rayleigh")],
            user_tau=[0.0, 0.2],
            user_mu=USER_MU,
            nstr=16,
            umu0=0.080442,
            fbeam=math.pi,
        ),
        flux=[
            [0.125561, 0.252716],
            [0, 0.127154],
        ],
        detail=[
            [
                0.25272,
                -4.453e-11,
                0.12556,
                7.9443e-14,
                0.28471,
                -9.0236e-12,
                0.034712,
                0.25,
            ],
            [
                0.021031,
                0.10612,
                -2.9773e-11,
                1.2187e-14,
                0.043676,
                0.02287,
                -2.8257e-12,
                0.020805,
            ],
        ],
        radiance=[
            [0, 0, 0, 0.3477, 0.0487, 0.0189],
            [0.0186, 0.0464, 0.0678, 0, 0, 0],
        ],
    ),
    Case(
        problem=Problem(
            label="Test Problem 2c: thick, partial scattering",
            layers=[Layer(dtau=5.0, ssalb=0.5, phase="rayleigh")],
            user_tau=[0.0, 5.0],
            user_mu=USER_MU,
            nstr=16,
            umu0=0.080442,
            fbeam=math.pi,
        ),
        flux=[
            [0.062473, 0.252716],
            [0, 0.000251683],
        ],
        detail=[
            [
                0.25272,
                0,
                0.062473,
                1.6746,
                0.26652,
                -7.732e-18,
                0.016524,
                0.25,
            ],
            [
                2.5608e-28,
                0.00025168,
                -1.2588e-20,
                0.00017546,
                2.7926e-05,
                2.7926e-05,
                -1.367e-21,
                2.5332e-28,
            ],
        ],
        radiance=[
            [0, 0, 0, 0.16257, 0.024579, 0.01015],
            [0.00017, 3.9717e-05, 1.3247e-05, 0, 0, 0],
        ],
    ),
    Case(
        problem=Problem(
            label="Test Problem 2d: thick, conservative scattering",
            layers=[Layer(dtau=5.0, ssalb=1.0, phase="rayleigh")],
            user_tau=[0.0, 5.0],
            user_mu=USER_MU,
            nstr=16,
            umu0=0.080442,
            fbeam=math.pi,
        ),
        flux=[
            [0.225915, 0.252716],
            [0, 0.0268008],
        ],
        detail=[
            [
                0.25272,
                1.3043e-11,
                0.22592,
                8.3718e-14,
                0.30003,
                2.0094e-12,
                0.050032,
                0.25,
            ],
            [
                2.5608e-28,
                0.026801,
                1.0693e-11,
                1.0277e-15,
                0.0036832,
                0.0036832,
                1.0204e-12,
                2.5332e-28,
            ],
        ],
        radiance=[
            [0, 0, 0, 0.364, 0.0827, 0.0492],
            [0.0106, 0.0077, 0.0038, 0, 0, 0],
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
