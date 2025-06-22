""" Test isotropic scattering with pydisort."""
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
