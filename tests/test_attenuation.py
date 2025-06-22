""" Test attenuation with pydisort."""
# pylint: disable = no-name-in-module, invalid-name,
# import-error, wrong-import-position

import torch
from numpy.testing import assert_equal, assert_allclose
from pydisort import DisortOptions, Disort


def test_attenuation():
    op = DisortOptions().header("Attenuation Test")
    op.flags("onlyfl,lamber")
    op.ds().nlyr = 4
    op.ds().nmom = 8
    op.ds().nstr = 8
    op.ds().nphase = 8

    ds = Disort(op)
    tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).unsqueeze(-1)
    result = ds.forward(tau, fbeam=torch.tensor([3.14159]))
    assert_equal(result.shape, (1, 1, 5, 2))
    result.squeeze_()
    assert_allclose(
        result,
        torch.tensor(
            [
                [0.0000, 3.1416],
                [0.0000, 2.8426],
                [0.0000, 2.3273],
                [0.0000, 1.7241],
                [0.0000, 1.1557],
            ]
        ),
        atol=1e-4,
        rtol=1e-4,
    )
