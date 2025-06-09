# /usr/bin/env python

""" Test Problem 9: General Emitting/Absorbing/Scattering """

import torch
import numpy as np
from time import time
from numpy.testing import assert_allclose, assert_equal
from pydisort import (
    DisortOptions,
    Disort,
    scattering_moments,
)


def run_case1(ncol=1, nlyr=6, nstr=8, ssalb=0.05):
    op = DisortOptions().header("Test Problem 9, Case 1")
    op.flags(
        "usrtau,usrang,lamber,quiet,"
        "intensity_correction,old_intensity_correction,"
        # "print-input,print-phase-function"
    )

    op.ds().nlyr = nlyr
    op.ds().nmom = nstr
    op.ds().nstr = nstr
    op.ds().nphase = nstr

    op.user_tau(np.array([0.0, 1.05, 2.1, 6.0, 21.0]))
    op.user_mu(np.array([-1.0, -0.2, 0.2, 1.0]))
    op.user_phi(np.array([60.0]))
    op.accur(0.0)
    op.ncol(ncol)
    op.nwave(1)

    ds = Disort(op)
    nprop = 2 + op.ds().nmom

    # set boundary conditions
    bc = {
        "umu0": torch.tensor([0.5] * ncol),
        "phi0": torch.tensor([0.0] * ncol),
        "fbeam": torch.tensor([[0.0] * ncol]),
        "fluor": torch.tensor([[0.0] * ncol]),
        "fisot": torch.tensor([[1.0 / np.pi] * ncol]),
    }
    bc["albedo"] = torch.tensor([[0.0] * ncol])

    tau = torch.zeros((ncol, op.ds().nlyr, nprop))
    tau[:, :, 0] = torch.arange(1, op.ds().nlyr + 1) / nlyr * 6.0
    tau[:, :, 1] = 0.6 + torch.arange(1, op.ds().nlyr + 1) * ssalb
    tau[:, :, 2:] = scattering_moments(op.ds().nmom, "isotropic")

    # up and down flux
    ds.forward(tau, **bc)
    return ds


def test_case1():
    ds = run_case1(ncol=1, nlyr=6, nstr=8, ssalb=0.05)

    # get radiance
    rad = ds.gather_rad()
    assert_equal(rad.shape, (1, 1, 1, 5, 4))
    rad.squeeze_()
    assert_allclose(
        rad,
        torch.tensor(
            [
                [3.1831e-01, 3.1831e-01, 9.9892e-02, 5.9134e-02],
                [1.5351e-01, 5.0953e-02, 3.6701e-02, 2.3190e-02],
                [7.0661e-02, 2.0912e-02, 1.4854e-02, 9.7231e-03],
                [3.7278e-03, 1.0882e-03, 8.8332e-04, 5.9474e-04],
                [2.8766e-07, 1.0592e-07, 0.0000e00, 0.0000e00],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )


def speed_test_case1():
    pass


if __name__ == "__main__":
    torch.set_default_dtype(torch.float64)

    # time the test
    start_time = time()
    run_case1(ncol=20000, nlyr=100, nstr=32, ssalb=0.003)
    elapsed_time = time() - start_time
    print(f"Test completed in {elapsed_time:.2f} seconds.")
