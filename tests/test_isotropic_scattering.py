#!/usr/bin/env python

""" Test isotropic scattering with pydisort."""
# pylint: disable = no-name-in-module, invalid-name,
# import-error, wrong-import-position


import unittest
import torch
import numpy as np
from numpy.testing import assert_allclose
from pydisort import (
    DisortOptions,
    Disort,
    scattering_moments,
)


class PyDisortTests(unittest.TestCase):
    """Test unit: cdisort test01 isotropic scattering with pydisort."""

    def setUp(self):
        """Set up the test."""
        op = DisortOptions().header("test isotropic scattering")
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

        self.ds = Disort(op)
        self.nwave = 1
        self.ncol = 1
        self.nlyr = op.ds().nlyr
        self.nprop = 2 + op.ds().nmom

    def test_isotropic_scattering_01(self):
        """Test isotropic scattering, case No.1."""
        # set boundary conditions
        self.bc = {
            "umu0": torch.tensor([0.1]),
            "phi0": torch.tensor([0.0]),
            "albedo": torch.zeros((1, 1)),
            "fluor": torch.zeros((1, 1)),
            "fisot": torch.zeros((1, 1)),
        }
        self.bc["fbeam"] = np.pi / self.bc["umu0"].reshape((1, 1))

        tau = torch.zeros((1, self.nprop))
        tau[0, 0] = self.ds.options.user_tau()[-1]
        tau[0, 1] = 0.2

        # scattering moments
        tau[0, 2:] = scattering_moments(self.nprop - 2, "isotropic")

        # up and down flux
        result = self.ds.forward(tau, self.bc)
        self.assertEqual(result.shape, (1, 1, 2, 2))
        assert_allclose(
            result,
            torch.tensor(
                [[[[7.994510e-02, 3.141593e00], [0.0, 2.377849e00]]]]
            ),
            atol=1e-8,
            rtol=1e-5,
        )

        # all fluxes
        flx = self.ds.gather_flx()
        self.assertEqual(flx.shape, (1, 1, 2, 8))
        assert_allclose(
            flx,
            torch.tensor(
                [
                    [
                        [
                            [
                                3.141593e00,
                                -4.440892e-16,
                                7.994510e-02,
                                2.540670e01,
                                2.527252e00,
                                -3.726496e-16,
                                2.725154e-02,
                                2.5000e00,
                            ],
                            [
                                2.298438e00,
                                7.9411080e-02,
                                -2.532818e-17,
                                1.865321e01,
                                1.8555e00,
                                2.6421e-02,
                                -1.2560e-19,
                                1.8290e00,
                            ],
                        ]
                    ]
                ]
            ),
            atol=1e-4,
            rtol=1e-4,
        )

        # all radiance
        rad = self.ds.gather_rad()
        self.assertEqual(rad.shape, (1, 1, 1, 2, 6))
        assert_allclose(
            rad,
            torch.tensor(
                [
                    [
                        [
                            [
                                [
                                    0.0,
                                    0.0,
                                    0.0,
                                    0.11777066,
                                    0.02641704,
                                    0.01340413,
                                ],
                                [
                                    0.01338263,
                                    0.02633235,
                                    0.11589789,
                                    0.0,
                                    0.0,
                                    0.0,
                                ],
                            ]
                        ]
                    ]
                ]
            ),
            atol=1e-4,
            rtol=1e-4,
        )

    '''
    def test_isotropic_scattering_02(self):
        """Test isotropic scattering."""
        self.ds.options.header("01. test isotropic scattering")

        # case No.2
        print("==== Case No.2 ====")
        ds.set_single_scattering_albedo(array([1.0]))
        rad, flx = ds.run()
        assert_allclose(
            rad,
            array(
                [
                    [
                        [0.0, 0.0, 0.0, 0.62288378, 0.13976294, 0.07091916],
                        [0.07081093, 0.1393367, 0.61345786, 0.0, 0.0, 0.0],
                    ]
                ]
            ),
            atol=1e-8,
            rtol=1e-5,
        )

        flx = flx[:, [RFLDIR, FLDN, FLUP]]
        assert_allclose(
            flx,
            array(
                [
                    [3.14159265e00, 0., 4.22921778e-01],
                    [2.29843829e00, 4.20232590e-01, 0.],
                ]
            ),
            atol=1e-8,
            rtol=1e-5,
        )
    '''


if __name__ == "__main__":
    unittest.main()
