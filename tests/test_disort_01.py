"""Test isotropic scattering with pydisort.

This module contains 6 test cases corresponding to disort_test01() in the C implementation
(tests/cdisort213/test_cdisort.c). All tests verify isotropic scattering scenarios with
varying optical depths, single-scatter albedos, and source types.

Test Cases:
-----------
test_case1:
    Thin optical depth (tau=0.03125), low single-scatter albedo (0.2), beam source.
    Tests basic absorbing medium with minimal scattering.

test_case2:
    Thin optical depth (tau=0.03125), full scattering (albedo=1.0), beam source.
    Tests conservative scattering in thin medium.

test_case3:
    Thin optical depth (tau=0.03125), near-conservative scattering (albedo=0.99), 
    isotropic source. Tests diffuse illumination in thin medium.

test_case4:
    Thick optical depth (tau=32), low single-scatter albedo (0.2), beam source.
    Tests absorbing medium with significant optical depth.

test_case5:
    Thick optical depth (tau=32), full scattering (albedo=1.0), beam source.
    Tests conservative scattering in optically thick medium.

test_case6:
    Thick optical depth (tau=32), near-conservative scattering (albedo=0.99),
    isotropic source. Tests diffuse illumination in optically thick medium.

All tests verify:
- Upward and downward fluxes at user-specified optical depths
- Detailed flux outputs (direct beam, diffuse up/down, mean intensities)
- Radiance intensities at specified polar angles

Reference:
----------
Van de Hulst, H.C., 1980: Multiple Light Scattering, Tables, Formulas and Applications,
Volumes 1 and 2, Academic Press, New York (VH1, Table 12).
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
    """Case 1: Thin optical depth (tau=0.03125), low albedo (0.2), beam source.
    
    Tests basic absorbing medium with minimal scattering under direct beam illumination.
    Single-scatter albedo of 0.2 means most photons are absorbed rather than scattered.
    """
    op = DisortOptions().header("Isotropic Scattering Case 1")
    op.flags(
        "usrtau,usrang,lamber,quiet,"
        "intensity_correction,old_intensity_correction,"
        "print-input,print-phase-function"
    )

    op.ds().nlyr = 1
    op.ds().nmom = 16
    op.ds().nstr = 16
    op.ds().nphase = 16

    op.user_tau(np.array([0.0, 0.03125]))
    op.user_mu(np.array([-1.0, -0.5, -0.1, 0.1, 0.5, 1.0]))
    op.user_phi(np.array([0.0]))

    ds = Disort(op)
    nprop = 2 + op.ds().nmom

    # set boundary conditions
    bc = {
        "umu0": torch.tensor([0.1]),
        "phi0": torch.tensor([0.0]),
        "albedo": torch.tensor([0.0]),
        "fluor": torch.tensor([0.0]),
        "fisot": torch.tensor([0.0]),
    }
    bc["fbeam"] = np.pi / bc["umu0"]

    # scattering moments
    tau = torch.zeros((1, nprop))
    tau[0, 0] = ds.options.user_tau()[-1]
    tau[0, 1] = 0.2
    tau[0, 2:] = scattering_moments(nprop - 2, "isotropic")

    # up and down flux
    result = ds.forward(tau, **bc)
    assert_equal(result.shape, (1, 1, 2, 2))
    result.squeeze_()
    assert_allclose(
        result,
        torch.tensor([[7.994510e-02, 3.141593], [0.0, 2.377849]]),
        atol=1e-8,
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
                    3.14159,
                    0.0,
                    7.99451e-02,
                    2.54067e01,
                    2.52725,
                    0.0,
                    2.72515e-02,
                    2.5,
                ],
                [
                    2.29844,
                    7.941108e-02,
                    0.0,
                    1.865312e01,
                    1.8555,
                    2.6421e-02,
                    0.0,
                    1.829,
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
                [0.0, 0.0, 0.0, 0.11777066, 0.02641704, 0.01340413],
                [0.01338263, 0.02633235, 0.11589789, 0.0, 0.0, 0.0],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )


def test_case2():
    """Case 2: Thin optical depth (tau=0.03125), conservative scattering (albedo=1.0), beam source.
    
    Tests conservative scattering in thin medium where all photons are scattered (no absorption).
    Single-scatter albedo of 1.0 means no energy loss, only redistribution of radiation.
    """
    op = DisortOptions().header("Isotropic Scattering Case 2")
    op.flags(
        "usrtau,usrang,lamber,quiet,"
        "intensity_correction,old_intensity_correction,"
        "print-input,print-phase-function"
    )

    op.ds().nlyr = 1
    op.ds().nmom = 16
    op.ds().nstr = 16
    op.ds().nphase = 16

    op.user_tau(np.array([0.0, 0.03125]))
    op.user_mu(np.array([-1.0, -0.5, -0.1, 0.1, 0.5, 1.0]))
    op.user_phi(np.array([0.0]))

    ds = Disort(op)
    nprop = 2 + op.ds().nmom

    # set boundary conditions
    bc = {
        "umu0": torch.tensor([0.1]),
        "phi0": torch.tensor([0.0]),
        "albedo": torch.tensor([0.0]),
        "fluor": torch.tensor([0.0]),
        "fisot": torch.tensor([0.0]),
    }
    bc["fbeam"] = np.pi / bc["umu0"]

    # scattering moments
    tau = torch.zeros((1, nprop))
    tau[0, 0] = ds.options.user_tau()[-1]
    tau[0, 1] = 1.0
    tau[0, 2:] = scattering_moments(nprop - 2, "isotropic")

    # up and down flux
    result = ds.forward(tau, **bc)
    assert_equal(result.shape, (1, 1, 2, 2))
    result.squeeze_()
    assert_allclose(
        result,
        torch.tensor([[4.229218e-01, 3.141593], [0.0, 2.718671]]),
        atol=1e-8,
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
                    3.1416e00,
                    3.1875e-10,
                    4.2292e-01,
                    7.3777e-13,
                    2.6441e00,
                    4.3873e-11,
                    1.4407e-01,
                    2.5000e00,
                ],
                [
                    2.2984e00,
                    4.2023e-01,
                    -6.4374e-10,
                    5.4939e-13,
                    1.9689e00,
                    1.3989e-01,
                    -7.5073e-11,
                    1.8290e00,
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
                [0.0000, 0.0000, 0.0000, 0.6229, 0.1398, 0.0709],
                [0.0708, 0.1393, 0.6135, 0.0000, 0.0000, 0.0000],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )


def test_case3():
    """Case 3: Thin optical depth (tau=0.03125), near-conservative scattering (albedo=0.99), isotropic source.
    
    Tests diffuse illumination in thin medium with minimal absorption. Unlike cases 1-2 which
    use a directional beam, this case uses isotropic (uniform) incident radiation from above.
    """
    op = DisortOptions().header("Isotropic Scattering Case 3")
    op.flags(
        "usrtau,usrang,lamber,quiet,"
        "intensity_correction,old_intensity_correction,"
        "print-input,print-phase-function"
    )

    op.ds().nlyr = 1
    op.ds().nmom = 16
    op.ds().nstr = 16
    op.ds().nphase = 16

    op.user_tau(np.array([0.0, 0.03125]))
    op.user_mu(np.array([-1.0, -0.5, -0.1, 0.1, 0.5, 1.0]))
    op.user_phi(np.array([0.0]))

    ds = Disort(op)
    nprop = 2 + op.ds().nmom

    # set boundary conditions
    bc = {
        "umu0": torch.tensor([0.1]),
        "phi0": torch.tensor([0.0]),
        "albedo": torch.tensor([0.0]),
        "fluor": torch.tensor([0.0]),
        "fbeam": torch.tensor([0.0]),
    }
    bc["fisot"] = torch.tensor([1.0])

    # scattering moments
    tau = torch.zeros((1, nprop))
    tau[0, 0] = ds.options.user_tau()[-1]
    tau[0, 1] = 0.99
    tau[0, 2:] = scattering_moments(nprop - 2, "isotropic")

    # up and down flux
    result = ds.forward(tau, **bc)
    assert_equal(result.shape, (1, 1, 2, 2))
    result.squeeze_()
    assert_allclose(
        result,
        torch.tensor([[9.065564e-02, 3.141593e00], [0.0, 3.048975e00]]),
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
                    0.0000e00,
                    3.1416e00,
                    9.0656e-02,
                    6.6687e-02,
                    5.3068e-01,
                    5.0000e-01,
                    3.0679e-02,
                    0.0000e00,
                ],
                [
                    0.0000e00,
                    3.0490e00,
                    -3.2731e-18,
                    5.8894e-02,
                    4.6866e-01,
                    4.6866e-01,
                    -1.1800e-17,
                    0.0000e00,
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
                [1.0000, 1.0000, 1.0000, 0.1332, 0.0300, 0.0152],
                [0.9844, 0.9694, 0.8639, 0.0000, 0.0000, 0.0000],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )


def test_case4():
    """Case 4: Thick optical depth (tau=32), low albedo (0.2), beam source.
    
    Tests absorbing medium with significant optical depth. At tau=32, the medium is optically
    thick, meaning most radiation is absorbed/scattered before reaching the bottom. The low
    single-scatter albedo (0.2) means strong absorption dominates over scattering.
    """
    op = DisortOptions().header("Isotropic Scattering Case 4")
    op.flags(
        "usrtau,usrang,lamber,quiet,"
        "intensity_correction,old_intensity_correction,"
        "print-input,print-phase-function"
    )

    op.ds().nlyr = 1
    op.ds().nmom = 16
    op.ds().nstr = 16
    op.ds().nphase = 16

    op.user_tau(np.array([0.0, 32.0]))
    op.user_mu(np.array([-1.0, -0.5, -0.1, 0.1, 0.5, 1.0]))
    op.user_phi(np.array([0.0]))

    ds = Disort(op)
    nprop = 2 + op.ds().nmom

    # set boundary conditions
    bc = {
        "umu0": torch.tensor([0.1]),
        "phi0": torch.tensor([0.0]),
        "albedo": torch.tensor([0.0]),
        "fluor": torch.tensor([0.0]),
        "fisot": torch.tensor([0.0]),
    }
    bc["fbeam"] = np.pi / bc["umu0"]

    # scattering moments
    tau = torch.zeros((1, nprop))
    tau[0, 0] = ds.options.user_tau()[-1]
    tau[0, 1] = 0.2
    tau[0, 2:] = scattering_moments(nprop - 2, "isotropic")

    # up and down flux
    result = ds.forward(tau, **bc)
    assert_equal(result.shape, (1, 1, 2, 2))
    result.squeeze_()
    assert_allclose(
        result,
        torch.tensor([[2.59686e-01, 3.14159], [0.0, 0.0]]),
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
                    3.14159e00,
                    0.0000e00,
                    2.59686e-01,
                    2.57766e01,
                    2.56404e00,
                    0.0000e00,
                    6.40413e-02,
                    2.5000e00,
                ],
                [
                    0.0000e00,
                    0.0000e00,
                    0.0000e00,
                    0.0000e00,
                    0.0000e00,
                    0.0000e00,
                    0.0000e00,
                    0.0000e00,
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
                [0.0, 0.0, 0.0, 0.262972, 0.0906967, 0.0502853],
                [1.22980e-15, 1.30698e-17, 6.88840e-18, 0.0, 0.0, 0.0],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )


def test_case5():
    """Case 5: Thick optical depth (tau=32), conservative scattering (albedo=1.0), beam source.
    
    Tests conservative scattering in optically thick medium. With no absorption (albedo=1.0),
    radiation diffuses through multiple scattering events. This represents the extreme case
    of pure scattering with no energy loss in a thick atmosphere.
    """
    op = DisortOptions().header("Isotropic Scattering Case 5")
    op.flags(
        "usrtau,usrang,lamber,quiet,"
        "intensity_correction,old_intensity_correction,"
        "print-input,print-phase-function"
    )

    op.ds().nlyr = 1
    op.ds().nmom = 16
    op.ds().nstr = 16
    op.ds().nphase = 16

    op.user_tau(np.array([0.0, 32.0]))
    op.user_mu(np.array([-1.0, -0.5, -0.1, 0.1, 0.5, 1.0]))
    op.user_phi(np.array([0.0]))

    ds = Disort(op)
    nprop = 2 + op.ds().nmom

    # set boundary conditions
    bc = {
        "umu0": torch.tensor([0.1]),
        "phi0": torch.tensor([0.0]),
        "albedo": torch.tensor([0.0]),
        "fluor": torch.tensor([0.0]),
        "fisot": torch.tensor([0.0]),
    }
    bc["fbeam"] = np.pi / bc["umu0"]

    # scattering moments
    tau = torch.zeros((1, nprop))
    tau[0, 0] = ds.options.user_tau()[-1]
    tau[0, 1] = 1.0
    tau[0, 2:] = scattering_moments(nprop - 2, "isotropic")

    # up and down flux
    result = ds.forward(tau, **bc)
    assert_equal(result.shape, (1, 1, 2, 2))
    result.squeeze_()
    assert_allclose(
        result,
        torch.tensor([[3.07390, 3.14159], [0.0, 6.76954e-02]]),
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
                    3.14159e00,
                    0.0000e00,
                    3.07390e00,
                    0.0000e00,
                    3.10898e00,
                    0.0000e00,
                    6.09045e-01,
                    2.5000e00,
                ],
                [
                    0.0000e00,
                    6.76954e-02,
                    0.0000e00,
                    0.0000e00,
                    9.33064e-03,
                    9.33064e-03,
                    0.0000e00,
                    0.0000e00,
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
                [0.0, 0.0, 0.0, 1.93321, 1.02732, 0.797199],
                [0.0271316, 0.0187805, 0.0116385, 0.0, 0.0, 0.0],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )


def test_case6():
    """Case 6: Thick optical depth (tau=32), near-conservative scattering (albedo=0.99), isotropic source.
    
    Tests diffuse illumination in optically thick medium with minimal absorption. Combines the
    challenges of thick optical depth with isotropic incident radiation. The near-unity albedo
    (0.99) means photons undergo many scattering events before being absorbed or exiting.
    """
    op = DisortOptions().header("Isotropic Scattering Case 6")
    op.flags(
        "usrtau,usrang,lamber,quiet,"
        "intensity_correction,old_intensity_correction,"
        "print-input,print-phase-function"
    )

    op.ds().nlyr = 1
    op.ds().nmom = 16
    op.ds().nstr = 16
    op.ds().nphase = 16

    op.user_tau(np.array([0.0, 32.0]))
    op.user_mu(np.array([-1.0, -0.5, -0.1, 0.1, 0.5, 1.0]))
    op.user_phi(np.array([0.0]))

    ds = Disort(op)
    nprop = 2 + op.ds().nmom

    # set boundary conditions
    bc = {
        "umu0": torch.tensor([0.1]),
        "phi0": torch.tensor([0.0]),
        "albedo": torch.tensor([0.0]),
        "fluor": torch.tensor([0.0]),
        "fbeam": torch.tensor([0.0]),
    }
    bc["fisot"] = torch.tensor([1.0])

    # scattering moments
    tau = torch.zeros((1, nprop))
    tau[0, 0] = ds.options.user_tau()[-1]
    tau[0, 1] = 0.99
    tau[0, 2:] = scattering_moments(nprop - 2, "isotropic")

    # up and down flux
    result = ds.forward(tau, **bc)
    assert_equal(result.shape, (1, 1, 2, 2))
    result.squeeze_()
    assert_allclose(
        result,
        torch.tensor([[2.49618, 3.14159], [0.0, 4.60048e-03]]),
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
                    0.0000e00,
                    3.14159e00,
                    2.49618e00,
                    1.14239e-01,
                    9.09090e-01,
                    5.00000e-01,
                    4.09090e-01,
                    0.0000e00,
                ],
                [
                    0.0000e00,
                    4.60048e-03,
                    0.0000e00,
                    7.93633e-05,
                    6.31548e-04,
                    6.31548e-04,
                    0.0000e00,
                    0.0000e00,
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
                [1.0, 1.0, 1.0, 0.87751, 0.815136, 0.752715],
                [0.00186840, 0.00126492, 0.00077928, 0.0, 0.0, 0.0],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )
