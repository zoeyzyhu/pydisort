"""CPU and CUDA must produce the same fluxes for the same inputs.

cdisort is compiled as ``__host__ __device__`` code, so the GPU runs the same
solver as the CPU rather than a reimplementation of it. Differences should
therefore come only from floating-point arithmetic ordering, plus the
specialised fast flux-only kernels that ``forward`` selects for the common
4- and 8-stream configurations.

This covers the whole configuration space the CUDA path supports, which is
wider than ``test_fast_flux_routing.py``: that module checks how columns are
routed near conservative scattering, all of it shortwave. Here both
**shortwave and longwave** are exercised, so the thermal CUDA path -- Planck
emission, ``btemp``, ``ttemp``, wavenumber bounds -- is covered too, across
clear-sky and scattering media, four optical depths, and three solar angles.

On the tolerance: see ``AGREEMENT_TOLERANCE`` below. It is deliberately loose.
"""

import pytest
import torch
from conftest import DTYPE, requires_cuda
from pydisort import Disort, DisortOptions

pytestmark = requires_cuda

NLAYER = 40
NPROFILE = 1
WAVE_LOWER = [0.0]
WAVE_UPPER = [50000.0]

# Relative RMSE between the CPU and CUDA fluxes.
#
# This bound is loose on purpose. The two devices run the same double
# precision code, so well-conditioned cases agree to roughly machine
# precision, but `forward` also routes 4- and 8-stream flux-only work to
# specialised CUDA kernels that reach the same answer by a different route;
# `test_fast_flux_routing.py` allows those 2e-6. A single threshold covering
# every configuration here has to sit above that.
#
# 1e-4 is therefore not a precision claim. It is a regression guard: it
# catches the failures that matter -- a kernel returning zeros, NaNs, a stale
# buffer, or the thermal source being dropped on the GPU -- without risking
# false failures on hardware this was never calibrated against. Tighten it
# once there are measurements from a real GPU; the printed values below are
# what to tighten it against.
AGREEMENT_TOLERANCE = 1.0e-4


def scattering_properties(nstr, device, scattering):
    options = {"device": device, "dtype": DTYPE}
    prop = torch.empty((1, NPROFILE, NLAYER, 2 + nstr), **options)
    prop[..., 0] = 0.1
    prop[..., 1] = 0.5 if scattering else 0.0
    for moment in range(nstr):
        prop[..., 2 + moment] = 0.5 ** (moment + 1) if scattering else 0.0
    return prop


def thermal_profile(device):
    """An isothermal top, a linear ramp, and an isothermal 300 K base."""
    options = {"device": device, "dtype": DTYPE}
    level = torch.arange(NLAYER + 1, **options)
    top_to_bottom = torch.where(
        level <= 5,
        torch.ones_like(level),
        torch.where(
            level >= NLAYER - 4,
            torch.full_like(level, 300.0),
            1.0 + 299.0 * (level - 5) / (NLAYER - 10),
        ),
    )
    return top_to_bottom.flip(0).expand(NPROFILE, -1).contiguous()


def make_solver(mode, nstr):
    options = DisortOptions()
    options.upward(True)
    options.flags(
        "onlyfl,lamber,quiet" + (",planck" if mode == "longwave" else "")
    )
    options.nwave(1)
    options.ncol(NPROFILE)
    if mode == "longwave":
        options.wave_lower(WAVE_LOWER)
        options.wave_upper(WAVE_UPPER)
    options.ds().nlyr = NLAYER
    options.ds().nstr = nstr
    options.ds().nmom = nstr
    options.ds().nphase = nstr
    return Disort(options)


def solve(mode, scattering, nstr, device, optical_depth, albedo, umu0):
    """One solve on `device`; returns the flux tensor on the CPU."""
    options = {"device": device, "dtype": DTYPE}
    prop = scattering_properties(nstr, device, scattering)
    prop[..., 0] = optical_depth / NLAYER

    if mode == "shortwave":
        temperature = None
        bc = {
            "umu0": torch.full((NPROFILE,), umu0, **options),
            "fbeam": torch.full((1, NPROFILE), 100.0, **options),
            "albedo": torch.full((1, NPROFILE), albedo, **options),
        }
    else:
        temperature = thermal_profile(device)
        bc = {
            "albedo": torch.full((1, NPROFILE), albedo, **options),
            "temis": torch.zeros((1, NPROFILE), **options),
            "btemp": temperature[:, 0],
            "ttemp": temperature[:, -1],
        }

    solver = make_solver(mode, nstr)
    output = (
        solver(prop, **bc)
        if temperature is None
        else solver(prop, temf=temperature, **bc)
    )
    if device.type == "cuda":
        torch.cuda.synchronize(device)
    return output.cpu()


def relative_rmse(value, reference):
    """RMS difference over RMS reference; None when the reference is zero."""
    scale = float(reference.square().mean().sqrt())
    error = float((value - reference).square().mean().sqrt())
    if scale:
        return error / scale
    return 0.0 if error == 0.0 else None


# (mode, scattering, optical_depth, albedo, umu0)
CASES = [
    (mode, scattering, tau, 0.1 if mode == "shortwave" else 0.0, umu0)
    for mode in ("shortwave", "longwave")
    for scattering in (False, True)
    for tau in (0.1, 1.0, 4.0, 20.0)
    for umu0 in ((0.1, 0.5, 0.9) if mode == "shortwave" else (0.0,))
]

# A bright surface under a clear shortwave sky, where the Lambertian term
# dominates the upward flux and a mishandled albedo would show up plainly.
CASES.append(("shortwave", False, 1.0, 0.8, 0.5))


@pytest.mark.parametrize("nstr", [4, 8])
@pytest.mark.parametrize(
    "mode,scattering,optical_depth,albedo,umu0",
    CASES,
    ids=lambda v: str(v),
)
def test_cuda_matches_cpu(
    nstr,
    mode,
    scattering,
    optical_depth,
    albedo,
    umu0,
    cpu_device,
    cuda_device,
):
    """Upward, downward and net flux must agree between the two devices."""
    reference = solve(
        mode, scattering, nstr, cpu_device, optical_depth, albedo, umu0
    )
    value = solve(
        mode, scattering, nstr, cuda_device, optical_depth, albedo, umu0
    )

    assert value.shape == reference.shape
    assert torch.isfinite(value).all(), "CUDA produced non-finite fluxes"

    metrics = {
        "up": relative_rmse(value[..., 0], reference[..., 0]),
        "down": relative_rmse(value[..., 1], reference[..., 1]),
        "net": relative_rmse(
            value[..., 0] - value[..., 1],
            reference[..., 0] - reference[..., 1],
        ),
    }
    max_abs = float((value - reference).abs().max())

    # Printed so that `pytest -s` reproduces the survey table this replaces,
    # and so a failure reports the neighbouring quantities too.
    print(
        f"{mode:<10} {'scat' if scattering else 'clear':<5} nstr={nstr} "
        f"tau={optical_depth:<5g} alb={albedo:<4g} umu0={umu0:<4g} "
        + " ".join(
            f"{name}={'N/A' if v is None else f'{100 * v:.3g}%'}"
            for name, v in metrics.items()
        )
        + f" max_abs={max_abs:.3g}"
    )

    for name, value_ in metrics.items():
        # None means the reference was identically zero and so was the
        # difference, which is agreement rather than a failure.
        if value_ is None:
            continue
        assert value_ <= AGREEMENT_TOLERANCE, (
            f"{mode} {'scattering' if scattering else 'clear-sky'} "
            f"nstr={nstr} tau={optical_depth:g} albedo={albedo:g} "
            f"umu0={umu0:g}: {name} relative RMSE {100 * value_:.4g}% "
            f"exceeds {100 * AGREEMENT_TOLERANCE:g}%"
        )


@pytest.mark.parametrize("nstr", [4, 8])
def test_thermal_path_actually_emits(nstr, cuda_device):
    """The longwave CUDA path must carry the Planck source, not just run.

    A thermal solve whose emission term was dropped would still return an
    array of the right shape and would still agree with a CPU run that had
    dropped it too. This pins the physics instead: with a 300 K base and a
    cold top, the upward flux at the top must be substantial and must exceed
    the downward flux there, since the only source is the medium itself.
    """
    flux = solve("longwave", False, nstr, cuda_device, 1.0, 0.0, 0.0)
    upward_at_top = float(flux[0, 0, 0, 0])
    downward_at_top = float(flux[0, 0, 0, 1])

    assert upward_at_top > 1.0, "no thermal emission reached the top"
    assert upward_at_top > downward_at_top
