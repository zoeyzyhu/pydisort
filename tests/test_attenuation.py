#!/usr/bin/env python

""" Test attenuation with pydisort."""
# pylint: disable = no-name-in-module, invalid-name,
# import-error, wrong-import-position

import unittest
import torch
from numpy.testing import assert_allclose
from pydisort import DisortOptions, Disort


class PyDisortTests(unittest.TestCase):
    """Test unit: isotropic scattering with pydisort."""

    def setUp(self):
        op = DisortOptions().flags("onlyfl,lamber")
        op.ds().nlyr = 4
        op.ds().nmom = 8
        op.ds().nstr = 8
        op.ds().nphase = 8

        self.ds = Disort(op)

    def test_attenuation(self):
        """Test attenuation with pydisort."""
        tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).reshape((4, 1))
        bc = {"fbeam": torch.tensor([3.14159]).reshape((1, 1))}
        result = self.ds.forward(tau, bc)
        assert_allclose(
            result,
            torch.tensor(
                [
                    [
                        [
                            [0.0000, 3.1416],
                            [0.0000, 2.8426],
                            [0.0000, 2.3273],
                            [0.0000, 1.7241],
                            [0.0000, 1.1557],
                        ]
                    ]
                ]
            ),
            atol=1e-4,
            rtol=1e-4,
        )


if __name__ == "__main__":
    unittest.main()
