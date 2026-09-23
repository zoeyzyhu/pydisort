"""The CUDA fast flux-only paths must agree with the general solver.

pydisort has specialised CUDA kernels for the common flux-only configuration
(plane-parallel, Lambertian, no user angles or depths) at 4 and 8 streams.
They are faster than the general solver but only valid away from conservative
scattering, so ``forward`` routes any column with a single-scattering albedo
within ``1e-8`` of 1 to the general path instead.

That routing decision is what these tests pin down. Getting it wrong is quiet:
a column that should have taken the general path still returns plausible
numbers, just less accurate ones, and only near omega = 1 where the fast
kernel is ill-conditioned.

Every case compares the CUDA result against the CPU result for identical
inputs, so the CPU solver is the reference. Two tolerances apply, matching the
two paths:

* ``FAST_RELATIVE_TOLERANCE`` for columns the fast kernel legitimately
  handles, which is looser because that kernel takes a different route to the
  same answer;
* ``GENERAL_RELATIVE_TOLERANCE`` for columns routed to the general solver,
  which runs the same algorithm on both devices and should agree far more
  closely.
"""

import pytest
import torch
from conftest import DTYPE, requires_cuda
from pydisort import Disort, DisortOptions

pytestmark = requires_cuda

NLYR = 20

# forward() sends a column to the general solver when its single-scattering
# albedo is within this of 1; see kConservativeScatteringThreshold in
# src/disort.cpp.
CONSERVATIVE_THRESHOLD = 1.0e-8

FAST_RELATIVE_TOLERANCE = 2.0e-6
GENERAL_RELATIVE_TOLERANCE = 1.0e-8


def make_solver(nwave, ncol, nstr):
    options = DisortOptions()
    options.upward(True)
    options.flags("onlyfl,lamber,quiet")
    options.nwave(nwave)
    options.ncol(ncol)
    options.ds().nlyr = NLYR
    options.ds().nstr = nstr
    options.ds().nmom = nstr
    options.ds().nphase = nstr
    return Disort(options)


def make_inputs(nwave, ncol, ssalb, nstr):
    device = ssalb.device
    prop = torch.empty(
        (nwave, ncol, NLYR, 2 + nstr), device=device, dtype=DTYPE
    )
    prop[..., 0] = 0.5
    prop[..., 1] = ssalb
    for moment in range(nstr):
        prop[..., 2 + moment] = 0.85 ** (moment + 1)

    wave = torch.arange(nwave, device=device, dtype=DTYPE).view(nwave, 1)
    col = torch.arange(ncol, device=device, dtype=DTYPE).view(1, ncol)
    return prop, {
        "umu0": 0.5 + 0.05 * torch.arange(ncol, device=device, dtype=DTYPE),
        "fbeam": 100.0 + 7.0 * wave + col,
        "albedo": torch.zeros((nwave, ncol), device=device, dtype=DTYPE),
    }


def compare_case(ssalb, nstr):
    """CUDA against CPU for one albedo field. Returns (per-column, rel)."""
    nwave, ncol, _ = ssalb.shape
    solver = make_solver(nwave, ncol, nstr)

    prop_cuda, bc_cuda = make_inputs(nwave, ncol, ssalb.cuda(), nstr)
    gpu = solver(prop_cuda, **bc_cuda)
    torch.cuda.synchronize()

    cpu = solver(
        prop_cuda.cpu(),
        **{name: value.cpu() for name, value in bc_cuda.items()},
    )

    difference = (gpu.cpu() - cpu).abs()
    scale = float(cpu.abs().max())
    # Normalise by the field, not pointwise: deep levels carry fluxes many
    # orders of magnitude below the incident beam, where a relative tolerance
    # would measure rounding rather than the routing decision.
    return difference.amax(dim=(-1, -2)), float(difference.max()) / max(
        scale, 1.0
    )


@pytest.mark.parametrize("nstr", [4, 8])
@pytest.mark.parametrize("ssalb", [0.5, 0.9, 0.99, 0.9999])
def test_regular_columns_take_the_fast_path(nstr, ssalb):
    """Well away from omega = 1, the fast kernel must match the CPU."""
    field = torch.full((1, 1, NLYR), ssalb, dtype=DTYPE)
    _, relative = compare_case(field, nstr)
    assert relative <= FAST_RELATIVE_TOLERANCE, (
        f"nstr={nstr} ssalb={ssalb}: relative difference {relative:.3e} "
        f"exceeds {FAST_RELATIVE_TOLERANCE:.0e}"
    )


@pytest.mark.parametrize("nstr", [4, 8])
@pytest.mark.parametrize("deficit", [1e-8, 1e-9, 1e-10, 1e-12, 0.0])
def test_routing_switches_at_the_conservative_threshold(nstr, deficit):
    """Either side of the threshold, the right tolerance must hold.

    This is the case that matters. A column at ``1 - 1e-12`` is close enough
    to conservative that the fast kernel loses accuracy, so `forward` must
    route it to the general solver, where CPU and CUDA run the same algorithm
    and agree to ``1e-8``. If the routing threshold regressed, these columns
    would silently come back on the fast path and only meet the looser bound.
    """
    field = torch.full((1, 1, NLYR), 1.0 - deficit, dtype=DTYPE)
    _, relative = compare_case(field, nstr)

    expected = (
        FAST_RELATIVE_TOLERANCE
        if deficit >= CONSERVATIVE_THRESHOLD
        else GENERAL_RELATIVE_TOLERANCE
    )
    assert relative <= expected, (
        f"nstr={nstr} 1-ssalb={deficit:g}: relative difference "
        f"{relative:.3e} exceeds {expected:.0e}"
    )


@pytest.mark.parametrize("nstr", [4, 8])
def test_fully_conservative_batch_uses_the_general_solver(nstr):
    """A batch that is conservative throughout must route wholesale."""
    field = torch.ones((2, 4, NLYR), dtype=DTYPE)
    _, relative = compare_case(field, nstr)
    assert relative <= GENERAL_RELATIVE_TOLERANCE, (
        f"nstr={nstr}: conservative batch relative difference "
        f"{relative:.3e} exceeds {GENERAL_RELATIVE_TOLERANCE:.0e}"
    )


@pytest.mark.parametrize("nstr", [4, 8])
def test_mixed_batch_routes_per_column(nstr):
    """Routing is per column, not per batch.

    Here one batch holds ordinary columns alongside near-conservative ones.
    Every column must meet the tighter bound, which only happens if the
    conservative ones were individually diverted to the general solver rather
    than the whole batch being decided by its first element.
    """
    field = (
        torch.tensor(
            [[0.5, 1.0 - 1e-9, 0.9, 1.0], [0.9999, 0.99, 1.0 - 1e-12, 0.7]],
            dtype=DTYPE,
        )
        .unsqueeze(-1)
        .expand(2, 4, NLYR)
        .contiguous()
    )
    per_column, relative = compare_case(field, nstr)

    assert per_column.shape == (2, 4)
    assert relative <= GENERAL_RELATIVE_TOLERANCE, (
        f"nstr={nstr}: mixed batch relative difference {relative:.3e} "
        f"exceeds {GENERAL_RELATIVE_TOLERANCE:.0e}"
    )
