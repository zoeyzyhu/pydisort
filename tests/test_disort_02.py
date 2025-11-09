"""Test Rayleigh scattering with pydisort.

This module contains 4 test cases corresponding to disort_test02() in the C
implementation (tests/cdisort213/test_cdisort.c). All tests verify Rayleigh
scattering scenarios with varying optical depths and single-scatter albedos
under beam source illumination.

Test Cases:
-----------
test_case1:
    Moderate optical depth (tau=0.2), partial scattering (albedo=0.5), beam
    source. Tests Rayleigh scattering with balanced absorption and scattering.

test_case2:
    Moderate optical depth (tau=0.2), conservative scattering (albedo=1.0),
    beam source. Tests Rayleigh scattering with no absorption.

test_case3:
    Thick optical depth (tau=5.0), partial scattering (albedo=0.5), beam
    source. Tests Rayleigh scattering in optically thick medium with
    absorption.

test_case4:
    Thick optical depth (tau=5.0), conservative scattering (albedo=1.0), beam
    source. Tests Rayleigh scattering in optically thick medium without
    absorption.

All tests verify:
- Upward and downward fluxes at user-specified optical depths
- Detailed flux outputs (direct beam, diffuse up/down, mean intensities)
- Radiance intensities at specified polar angles

Reference:
----------
Coulson, K.L., J.V. Dave, and Z. Sekera, 1960: Tables Related to Radiation
Emerging from a Planetary Atmosphere with Rayleigh Scattering, Univ. of Calif.
Press, Berkeley (SW, Table 1).
"""

# pylint: disable = no-name-in-module, invalid-name,
# import-error, wrong-import-position

import torch
import numpy as np
from numpy.testing import assert_allclose, assert_equal
from pydisort import (
    DisortOptions,
    Disort,
    scattering_moments,
)


def test_case1():
    """Case 1: Moderate optical depth (tau=0.2), partial scattering
    (albedo=0.5), beam source.

    Tests Rayleigh scattering with balanced absorption and scattering.
    Rayleigh scattering is characteristic of molecular scattering where
    intensity varies with angle (preferential forward/backward scattering).
    """
    op = DisortOptions().header("Rayleigh Scattering Case 1")
    op.flags(
        "usrtau,usrang,lamber,quiet,"
        "intensity_correction,old_intensity_correction,"
        "print-input,print-phase-function"
    )

    op.ds().nlyr = 1
    op.ds().nmom = 16
    op.ds().nstr = 16
    op.ds().nphase = 16

    op.user_tau(np.array([0.0, 0.2]))
    op.user_mu(
        np.array(
            [-0.981986, -0.538263, -0.018014, 0.018014, 0.538263, 0.981986]
        )
    )
    op.user_phi(np.array([0.0]))

    ds = Disort(op)
    nprop = 2 + op.ds().nmom

    # set boundary conditions
    bc = {
        "umu0": torch.tensor([0.080442]),
        "phi0": torch.tensor([0.0]),
        "albedo": torch.tensor([0.0]),
        "fluor": torch.tensor([0.0]),
        "fisot": torch.tensor([0.0]),
    }
    bc["fbeam"] = torch.tensor([np.pi])

    # scattering moments for Rayleigh scattering
    tau = torch.zeros((1, nprop))
    tau[0, 0] = ds.options.user_tau()[-1]
    tau[0, 1] = 0.5
    tau[0, 2:] = scattering_moments(nprop - 2, "rayleigh")

    # up and down flux
    result = ds.forward(tau, **bc)
    assert_equal(result.shape, (1, 1, 2, 2))
    result.squeeze_()
    assert_allclose(
        result,
        torch.tensor([[5.35063e-02, 2.52716e-01], [0.0, 6.52102e-02]]),
        atol=1e-5,
        rtol=1e-5,
    )

    # all fluxes
    flx = ds.gather_flx()
    assert_equal(flx.shape, (1, 1, 2, 8))
    flx.squeeze_()
    assert_allclose(
        flx,
        torch.tensor(
            [
                [
                    2.5272e-01,
                    0.0000e00,
                    5.3506e-02,
                    1.6657e00,
                    2.6510e-01,
                    -7.7061e-18,
                    1.5104e-02,
                    2.5000e-01,
                ],
                [
                    2.1031e-02,
                    4.4179e-02,
                    -1.6106e-18,
                    1.8985e-01,
                    3.0215e-02,
                    9.4101e-03,
                    -2.2606e-19,
                    2.0805e-02,
                ],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )

    # all radiance
    rad = ds.gather_rad()
    assert_equal(rad.shape, (1, 1, 1, 2, 6))
    rad.squeeze_()
    assert_allclose(
        rad,
        torch.tensor(
            [
                [0.0000, 0.0000, 0.0000, 0.1618, 0.0212, 0.0079],
                [0.0077, 0.0201, 0.0258, 0.0000, 0.0000, 0.0000],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )


def test_case2():
    """Case 2: Moderate optical depth (tau=0.2), conservative scattering
    (albedo=1.0), beam source.

    Tests Rayleigh scattering with no absorption. All energy is redistributed
    by scattering according to the Rayleigh phase function.
    """
    op = DisortOptions().header("Rayleigh Scattering Case 2")
    op.flags(
        "usrtau,usrang,lamber,quiet,"
        "intensity_correction,old_intensity_correction,"
        "print-input,print-phase-function"
    )

    op.ds().nlyr = 1
    op.ds().nmom = 16
    op.ds().nstr = 16
    op.ds().nphase = 16

    op.user_tau(np.array([0.0, 0.2]))
    op.user_mu(
        np.array(
            [-0.981986, -0.538263, -0.018014, 0.018014, 0.538263, 0.981986]
        )
    )
    op.user_phi(np.array([0.0]))

    ds = Disort(op)
    nprop = 2 + op.ds().nmom

    # set boundary conditions
    bc = {
        "umu0": torch.tensor([0.080442]),
        "phi0": torch.tensor([0.0]),
        "albedo": torch.tensor([0.0]),
        "fluor": torch.tensor([0.0]),
        "fisot": torch.tensor([0.0]),
    }
    bc["fbeam"] = torch.tensor([np.pi])

    # scattering moments for Rayleigh scattering
    tau = torch.zeros((1, nprop))
    tau[0, 0] = ds.options.user_tau()[-1]
    tau[0, 1] = 1.0
    tau[0, 2:] = scattering_moments(nprop - 2, "rayleigh")

    # up and down flux
    result = ds.forward(tau, **bc)
    assert_equal(result.shape, (1, 1, 2, 2))
    result.squeeze_()
    assert_allclose(
        result,
        torch.tensor([[1.25561e-01, 2.52716e-01], [0.0, 1.27154e-01]]),
        atol=1e-5,
        rtol=1e-5,
    )

    # all fluxes
    flx = ds.gather_flx()
    assert_equal(flx.shape, (1, 1, 2, 8))
    flx.squeeze_()
    assert_allclose(
        flx,
        torch.tensor(
            [
                [
                    2.5272e-01,
                    -4.4530e-11,
                    1.2556e-01,
                    7.9443e-14,
                    2.8471e-01,
                    -9.0236e-12,
                    3.4712e-02,
                    2.5000e-01,
                ],
                [
                    2.1031e-02,
                    1.0612e-01,
                    -2.9773e-11,
                    1.2187e-14,
                    4.3676e-02,
                    2.2870e-02,
                    -2.8257e-12,
                    2.0805e-02,
                ],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )

    # all radiance
    rad = ds.gather_rad()
    assert_equal(rad.shape, (1, 1, 1, 2, 6))
    rad.squeeze_()
    assert_allclose(
        rad,
        torch.tensor(
            [
                [0.0000, 0.0000, 0.0000, 0.3477, 0.0487, 0.0189],
                [0.0186, 0.0464, 0.0678, 0.0000, 0.0000, 0.0000],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )


def test_case3():
    """Case 3: Thick optical depth (tau=5.0), partial scattering
    (albedo=0.5), beam source.

    Tests Rayleigh scattering in optically thick medium with absorption.
    At tau=5.0, significant attenuation occurs through both absorption and
    scattering.
    """
    op = DisortOptions().header("Rayleigh Scattering Case 3")
    op.flags(
        "usrtau,usrang,lamber,quiet,"
        "intensity_correction,old_intensity_correction,"
        "print-input,print-phase-function"
    )

    op.ds().nlyr = 1
    op.ds().nmom = 16
    op.ds().nstr = 16
    op.ds().nphase = 16

    op.user_tau(np.array([0.0, 5.0]))
    op.user_mu(
        np.array(
            [-0.981986, -0.538263, -0.018014, 0.018014, 0.538263, 0.981986]
        )
    )
    op.user_phi(np.array([0.0]))

    ds = Disort(op)
    nprop = 2 + op.ds().nmom

    # set boundary conditions
    bc = {
        "umu0": torch.tensor([0.080442]),
        "phi0": torch.tensor([0.0]),
        "albedo": torch.tensor([0.0]),
        "fluor": torch.tensor([0.0]),
        "fisot": torch.tensor([0.0]),
    }
    bc["fbeam"] = torch.tensor([np.pi])

    # scattering moments for Rayleigh scattering
    tau = torch.zeros((1, nprop))
    tau[0, 0] = ds.options.user_tau()[-1]
    tau[0, 1] = 0.5
    tau[0, 2:] = scattering_moments(nprop - 2, "rayleigh")

    # up and down flux
    result = ds.forward(tau, **bc)
    assert_equal(result.shape, (1, 1, 2, 2))
    result.squeeze_()
    assert_allclose(
        result,
        torch.tensor([[6.24730e-02, 2.52716e-01], [0.0, 2.51683e-04]]),
        atol=1e-5,
        rtol=1e-5,
    )

    # all fluxes
    flx = ds.gather_flx()
    assert_equal(flx.shape, (1, 1, 2, 8))
    flx.squeeze_()
    assert_allclose(
        flx,
        torch.tensor(
            [
                [
                    2.5272e-01,
                    0.0000e00,
                    6.2473e-02,
                    1.6746e00,
                    2.6652e-01,
                    -7.7320e-18,
                    1.6524e-02,
                    2.5000e-01,
                ],
                [
                    2.5608e-28,
                    2.5168e-04,
                    -1.2588e-20,
                    1.7546e-04,
                    2.7926e-05,
                    2.7926e-05,
                    -1.3670e-21,
                    2.5332e-28,
                ],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )

    # all radiance
    rad = ds.gather_rad()
    assert_equal(rad.shape, (1, 1, 1, 2, 6))
    rad.squeeze_()
    assert_allclose(
        rad,
        torch.tensor(
            [
                [
                    0.0000e00,
                    0.0000e00,
                    0.0000e00,
                    1.6257e-01,
                    2.4579e-02,
                    1.0150e-02,
                ],
                [
                    1.7000e-04,
                    3.9717e-05,
                    1.3247e-05,
                    0.0000e00,
                    0.0000e00,
                    0.0000e00,
                ],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )


def test_case4():
    """Case 4: Thick optical depth (tau=5.0), conservative scattering
    (albedo=1.0), beam source.

    Tests Rayleigh scattering in optically thick medium without absorption.
    Radiation diffuses through multiple Rayleigh scattering events with
    preferential angular redistribution.
    """
    op = DisortOptions().header("Rayleigh Scattering Case 4")
    op.flags(
        "usrtau,usrang,lamber,quiet,"
        "intensity_correction,old_intensity_correction,"
        "print-input,print-phase-function"
    )

    op.ds().nlyr = 1
    op.ds().nmom = 16
    op.ds().nstr = 16
    op.ds().nphase = 16

    op.user_tau(np.array([0.0, 5.0]))
    op.user_mu(
        np.array(
            [-0.981986, -0.538263, -0.018014, 0.018014, 0.538263, 0.981986]
        )
    )
    op.user_phi(np.array([0.0]))

    ds = Disort(op)
    nprop = 2 + op.ds().nmom

    # set boundary conditions
    bc = {
        "umu0": torch.tensor([0.080442]),
        "phi0": torch.tensor([0.0]),
        "albedo": torch.tensor([0.0]),
        "fluor": torch.tensor([0.0]),
        "fisot": torch.tensor([0.0]),
    }
    bc["fbeam"] = torch.tensor([np.pi])

    # scattering moments for Rayleigh scattering
    tau = torch.zeros((1, nprop))
    tau[0, 0] = ds.options.user_tau()[-1]
    tau[0, 1] = 1.0
    tau[0, 2:] = scattering_moments(nprop - 2, "rayleigh")

    # up and down flux
    result = ds.forward(tau, **bc)
    assert_equal(result.shape, (1, 1, 2, 2))
    result.squeeze_()
    assert_allclose(
        result,
        torch.tensor([[2.25915e-01, 2.52716e-01], [0.0, 2.68008e-02]]),
        atol=1e-5,
        rtol=1e-5,
    )

    # all fluxes
    flx = ds.gather_flx()
    assert_equal(flx.shape, (1, 1, 2, 8))
    flx.squeeze_()
    assert_allclose(
        flx,
        torch.tensor(
            [
                [
                    2.5272e-01,
                    1.3043e-11,
                    2.2592e-01,
                    8.3718e-14,
                    3.0003e-01,
                    2.0094e-12,
                    5.0032e-02,
                    2.5000e-01,
                ],
                [
                    2.5608e-28,
                    2.6801e-02,
                    1.0693e-11,
                    1.0277e-15,
                    3.6832e-03,
                    3.6832e-03,
                    1.0204e-12,
                    2.5332e-28,
                ],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )

    # all radiance
    rad = ds.gather_rad()
    assert_equal(rad.shape, (1, 1, 1, 2, 6))
    rad.squeeze_()
    assert_allclose(
        rad,
        torch.tensor(
            [
                [0.0000, 0.0000, 0.0000, 0.3640, 0.0827, 0.0492],
                [0.0106, 0.0077, 0.0038, 0.0000, 0.0000, 0.0000],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )
