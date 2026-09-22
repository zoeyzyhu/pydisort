"""`scattering_moments` builds the Legendre series DISORT expands against.

The solver never sees a phase function directly; it sees this series. Every
scattering result in the suite therefore rests on these coefficients being
right, and the reference problems only exercise two of the six forms on offer.

Each moment has a closed form for the analytic types, so these are checked
against the formula rather than against recorded output. The two tabulated
types come from cdisort's own ``c_getmom``, so they are checked on the
properties a phase-function expansion must have.
"""

import math

import pytest
import torch
from numpy.testing import assert_allclose
from pydisort import Disort, DisortOptions, scattering_moments

TYPES = [
    "isotropic",
    "rayleigh",
    "henyey-greenstein",
    "double-henyey-greenstein",
    "haze-garcia-siewert",
    "cloud-garcia-siewert",
]


@pytest.mark.parametrize("phase", TYPES)
@pytest.mark.parametrize("npmom", [4, 8, 32])
def test_series_length_and_range(phase, npmom):
    """Moment 0 is excluded, and no moment may exceed unity in magnitude.

    pydisort's convention is that the series starts at moment 1: the zeroth is
    always 1 by normalisation and is filled in by the C++ layer. Getting this
    off by one would shift every coefficient and quietly change the phase
    function rather than raise anything.
    """
    moments = scattering_moments(npmom, phase, 0.5, 0.2, 0.7)
    assert moments.shape == (npmom,)
    assert torch.isfinite(moments).all()
    # A normalised phase function has |moment_k| <= moment_0 = 1.
    assert moments.abs().max().item() <= 1.0 + 1e-12


def test_isotropic_has_no_structure():
    """Isotropic scattering is the zeroth moment alone."""
    moments = scattering_moments(16, "isotropic")
    assert_allclose(moments.numpy(), 0.0, atol=0, rtol=0)


def test_rayleigh_has_only_the_second_moment():
    """The Rayleigh phase function is 1 + 0.1 * P2, and nothing else."""
    moments = scattering_moments(16, "rayleigh")
    expected = torch.zeros(16, dtype=moments.dtype)
    expected[1] = 0.1  # moment 2, since the series starts at moment 1
    assert_allclose(moments.numpy(), expected.numpy(), atol=0, rtol=0)


@pytest.mark.parametrize("gg", [-0.9, -0.25, 0.0, 0.25, 0.75, 0.9])
def test_henyey_greenstein_is_a_geometric_series(gg):
    """Moment k of a Henyey-Greenstein function is exactly g**k."""
    npmom = 24
    moments = scattering_moments(npmom, "henyey-greenstein", gg)
    expected = torch.tensor(
        [gg**k for k in range(1, npmom + 1)], dtype=moments.dtype
    )
    assert_allclose(moments.numpy(), expected.numpy(), atol=1e-15, rtol=1e-12)


def test_henyey_greenstein_at_zero_is_isotropic():
    """g = 0 is the isotropic phase function, and must produce its series."""
    assert_allclose(
        scattering_moments(12, "henyey-greenstein", 0.0).numpy(),
        scattering_moments(12, "isotropic").numpy(),
        atol=0,
        rtol=0,
    )


@pytest.mark.parametrize(
    "gg1,gg2,ff", [(0.8, -0.3, 0.6), (0.5, 0.1, 0.0), (0.5, 0.1, 1.0)]
)
def test_double_henyey_greenstein_is_the_weighted_sum(gg1, gg2, ff):
    """The two-term form is ff * g1**k + (1 - ff) * g2**k.

    The ff = 0 and ff = 1 endpoints must collapse onto the single-term form,
    which is what makes the weighting unambiguous.
    """
    npmom = 16
    moments = scattering_moments(
        npmom, "double-henyey-greenstein", gg1, gg2, ff
    )
    expected = torch.tensor(
        [ff * gg1**k + (1.0 - ff) * gg2**k for k in range(1, npmom + 1)],
        dtype=moments.dtype,
    )
    assert_allclose(moments.numpy(), expected.numpy(), atol=1e-15, rtol=1e-12)


@pytest.mark.parametrize(
    "phase", ["haze-garcia-siewert", "cloud-garcia-siewert"]
)
def test_tabulated_phase_functions_are_forward_peaked(phase):
    """The two tabulated aerosol functions scatter mainly forwards.

    Their coefficients come from cdisort's c_getmom, so there is no formula to
    compare against. What is knowable in advance is that both describe
    forward-peaked particles: the first moment, which is the asymmetry
    parameter, must be positive and well below one.
    """
    moments = scattering_moments(32, phase)
    asymmetry = moments[0].item()
    assert 0.0 < asymmetry < 1.0
    assert torch.isfinite(moments).all()


def test_cloud_is_more_forward_peaked_than_haze():
    """Cloud droplets are larger than haze particles, so they peak harder.

    This orders the two tables against each other, which catches them being
    swapped in a way that checking either alone cannot.
    """
    haze = scattering_moments(32, "haze-garcia-siewert")[0].item()
    cloud = scattering_moments(32, "cloud-garcia-siewert")[0].item()
    assert cloud > haze


def test_unknown_phase_function_is_rejected():
    """A typo must fail rather than silently scatter isotropically."""
    with pytest.raises(RuntimeError, match="unknown phase function"):
        scattering_moments(8, "not-a-phase-function")


@pytest.mark.parametrize("gg", [-1.0, 1.0, 1.5])
def test_out_of_range_asymmetry_is_rejected(gg):
    """|g| < 1 is required; the series diverges at the endpoints."""
    with pytest.raises(RuntimeError, match="bad input variable"):
        scattering_moments(8, "henyey-greenstein", gg)


def test_rayleigh_needs_room_for_its_second_moment():
    """Asking for fewer than two moments cannot represent Rayleigh."""
    with pytest.raises(RuntimeError, match="npmom"):
        scattering_moments(1, "rayleigh")


def test_moments_reach_the_solver():
    """A phase function must change the answer, not just the input tensor.

    Two solves differing only in asymmetry parameter must disagree, and the
    more forward-peaked one must reflect less back out of the top. Without
    this the rest of the module could pass while the coefficients were
    dropped on the way into cdisort.
    """

    def reflectance(gg):
        op = DisortOptions().header("moments").flags("usrtau,lamber,quiet")
        op.ds().nlyr = 1
        op.ds().nstr = 16
        op.ds().nmom = 16
        op.ds().nphase = 16
        op.user_tau([0.0, 1.0])
        op.accur(0.0)
        op.ncol(1)
        op.nwave(1)
        ds = Disort(op)

        prop = torch.zeros((1, 1, 1, 18), dtype=torch.float64)
        prop[..., 0] = 1.0
        prop[..., 1] = 1.0  # conservative, so all of it is redistributed
        prop[..., 2:] = scattering_moments(16, "henyey-greenstein", gg)

        flx = ds.forward(
            prop,
            umu0=torch.tensor([1.0], dtype=torch.float64),
            fbeam=torch.tensor([[math.pi]], dtype=torch.float64),
        )
        return flx[0, 0, 0, 0].item()

    isotropic = reflectance(0.0)
    forward_peaked = reflectance(0.9)

    assert forward_peaked < isotropic
