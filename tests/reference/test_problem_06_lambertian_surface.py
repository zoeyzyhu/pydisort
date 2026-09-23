"""DISORT Test Problem 6: no scattering, increasingly complex sources.

A purely absorbing layer under a beam, with the lower boundary made
progressively more interesting. The cases here are the ones with a Lambertian
surface, which is the configuration pydisort exposes; upstream continues into
cases 6d-6h with a Hapke BRDF and ``lamber = FALSE``, and pydisort has no way
to select a BRDF (``brdf_type`` is fixed at ``BRDF_NONE``), so those are out of
scope rather than omitted by oversight.

Case 6c isolates Lambertian reflection without scattering. Problems 9c and
10 also exercise reflection, in multilayer scattering and emitting media.

Reference:
    Expected values transcribed from disort_test06() in
    tests/cdisort213/test_cdisort.c.
"""

import pytest
import torch
from conftest import ATOL, RTOL, Case, Layer, Problem
from numpy.testing import assert_allclose, assert_equal

USER_MU = [-1.0, -0.1, 0.1, 1.0]
BEAM_FLUX = 200.0
UMU0 = 0.5

# Upstream sets nmom = 0 because nothing scatters. pydisort requires
# nmom >= nstr, so the moments are allocated and left at their isotropic
# values; with ssalb = 0 they are never consulted.
NSTR = 16


def _problem(label, dtau, user_tau, albedo):
    return Problem(
        label=label,
        layers=[Layer(dtau=dtau, ssalb=0.0)],
        user_tau=user_tau,
        user_mu=USER_MU,
        user_phi=[90.0],
        nstr=NSTR,
        umu0=UMU0,
        fbeam=BEAM_FLUX,
        albedo=albedo,
    )


CASES = [
    Case(
        # A transparent medium: the beam arrives undiminished and nothing is
        # scattered, reflected or emitted. Both output depths sit at tau = 0.
        problem=_problem(
            "Test Problem 6a: transparent medium",
            dtau=0.0,
            user_tau=[0.0, 0.0],
            albedo=0.0,
        ),
        flux=[[0.0, 100.0], [0.0, 100.0]],
        radiance=[[0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0]],
    ),
    Case(
        # Add optical depth. The beam attenuates as exp(-tau / umu0) and there
        # is still nothing to send radiation back up.
        problem=_problem(
            "Test Problem 6b: absorbing medium",
            dtau=1.0,
            user_tau=[0.0, 0.5, 1.0],
            albedo=0.0,
        ),
        flux=[
            [0.0, 100.0],
            [0.0, 3.67879e01],
            [0.0, 1.35335e01],
        ],
        radiance=[
            [0.0, 0.0, 0.0, 0.0],
            [0.0, 0.0, 0.0, 0.0],
            [0.0, 0.0, 0.0, 0.0],
        ],
    ),
    Case(
        # Add a Lambertian surface. The upward flux is now non-zero at every
        # depth, and it is the only thing in the reference suite that tests it.
        problem=_problem(
            "Test Problem 6c: Lambertian surface, albedo 0.5",
            dtau=1.0,
            user_tau=[0.0, 0.5, 1.0],
            albedo=0.5,
        ),
        flux=[
            [1.48450e00, 100.0],
            [2.99914e00, 3.67879e01],
            [6.76676e00, 1.35335e01],
        ],
        radiance=[
            [0.0, 0.0, 9.77882e-05, 7.92386e-01],
            [0.0, 0.0, 1.45131e-02, 1.30642e00],
            [0.0, 0.0, 2.15393e00, 2.15393e00],
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

    rad = ds.gather_rad()
    assert_equal(rad.shape, (1, 1, 1, ntau, numu))
    assert_allclose(
        rad.squeeze(),
        torch.tensor(case.radiance, dtype=torch.float64),
        atol=1e-5,
        rtol=RTOL,
    )


def test_surface_albedo_scales_the_reflected_flux(solve):
    """Upward flux at the surface must be albedo times what arrives there.

    A Lambertian surface reflects a fixed fraction of the downward flux, so
    this relation holds exactly and independently of the published tables. It
    is what makes the albedo path a real check rather than a transcription.
    """
    for albedo in (0.0, 0.25, 0.5, 0.75, 1.0):
        problem = _problem(
            f"albedo {albedo}", dtau=1.0, user_tau=[0.0, 1.0], albedo=albedo
        )
        _, flx = solve(problem)
        downward_at_base = flx[0, 0, -1, 1].item()
        upward_at_base = flx[0, 0, -1, 0].item()
        assert_allclose(
            upward_at_base, albedo * downward_at_base, atol=1e-10, rtol=1e-9
        )
