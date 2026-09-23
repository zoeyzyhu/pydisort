#!/usr/bin/env python3
"""Example 1: Attenuation of a collimated beam (Beer-Lambert law).

The simplest possible DISORT problem: a purely absorbing, non-emitting
plane-parallel medium illuminated from above by a collimated beam.  The direct
(unscattered) beam flux at optical depth ``tau`` must follow the Beer-Lambert
law exactly,

    F_dir(tau) = F_beam * mu0 * exp(-tau / mu0),

where ``F_beam`` is the flux measured normal to the beam and ``mu0`` is the
cosine of the solar zenith angle.  Because the answer is known analytically,
this example doubles as an end-to-end correctness check of the installation.

It also introduces the two things every pydisort program does:

1. describe the problem with a :class:`~pydisort.DisortOptions` object, and
2. run it by calling :meth:`~pydisort.Disort.forward` on a tensor of layer
   optical properties.

Run with::

    python example_01_beam_attenuation.py
"""

import numpy as np
import torch

from pydisort import Disort, DisortOptions, kIRFLDIR

# DISORT is a double-precision code; ask torch for float64 so that the
# comparison against the analytic solution is not limited by float32.
torch.set_default_dtype(torch.float64)

NLYR = 8  # number of atmospheric layers
NSTR = 8  # number of discrete-ordinate streams
DTAU = 0.25  # optical thickness of every layer
FBEAM = 1000.0  # beam flux normal to the beam [W m-2]

# Three columns, solved simultaneously, with different solar zenith angles.
MU0 = np.array([1.0, 0.5, 0.2])


def build_solver(ncol: int) -> Disort:
    """Configure a flux-only, Lambertian-surface DISORT solver."""
    op = DisortOptions().flags("onlyfl,lamber,quiet")

    # `onlyfl` -> compute fluxes only (no radiances); this is the fast path.
    # `lamber` -> Lambertian lower boundary (always required, see the docs).
    # `quiet`  -> suppress cdisort's internal chatter.

    op.ds().nlyr = NLYR
    op.ds().nstr = NSTR
    op.ds().nmom = NSTR
    op.ds().nphase = NSTR

    # Wavenumber and column dimensions are the two dimensions pydisort
    # parallelizes over.  Here we use one band and `ncol` columns.
    op.nwave(1)
    op.ncol(ncol)

    return Disort(op)


def main() -> None:
    ncol = MU0.size
    ds = build_solver(ncol)

    # Layer optical properties, shape (nwave, ncol, nlyr, nprop).  The last
    # dimension holds, in order: optical thickness, single-scattering albedo,
    # then `nmom` phase-function moments.  Leaving the albedo and the moments
    # at zero makes the medium purely absorbing.
    nprop = 2 + NSTR
    prop = torch.zeros((1, ncol, NLYR, nprop))
    prop[..., 0] = DTAU

    zeros_wc = torch.zeros((1, ncol))
    flx = ds.forward(
        prop,
        umu0=torch.from_numpy(MU0),
        phi0=torch.zeros(ncol),
        fbeam=torch.full((1, ncol), FBEAM),
        albedo=zeros_wc,  # black surface: nothing is reflected back up
        fisot=zeros_wc,
        fluor=zeros_wc,
    )

    # `forward` returns (nwave, ncol, nlvl, 2) with [upward, downward] flux.
    print(f"flux tensor shape: {tuple(flx.shape)}  (nwave, ncol, nlvl, 2)")

    # `gather_flx` exposes the full cdisort flux output, including the direct
    # beam component that we want to compare against Beer-Lambert.
    direct = ds.gather_flx()[0, :, :, kIRFLDIR].numpy()

    tau = DTAU * np.arange(NLYR + 1)
    analytic = FBEAM * MU0[:, None] * np.exp(-tau[None, :] / MU0[:, None])
    max_err = np.abs(direct - analytic).max()

    print()
    print("Direct beam flux [W m-2] vs. Beer-Lambert")
    print(f"{'tau':>6}" + "".join(f"{f'mu0={m:g}':>22}" for m in MU0))
    for k in range(NLYR + 1):
        row = "".join(
            f"{direct[i, k]:>11.4f}/{analytic[i, k]:<10.4f}"
            for i in range(ncol)
        )
        print(f"{tau[k]:>6.2f}" + row)

    print()
    print(f"maximum absolute error: {max_err:.3e} W m-2")

    # A black, purely absorbing medium has no upward flux anywhere.
    assert torch.allclose(flx[..., 0], torch.zeros_like(flx[..., 0]))
    # And with no scattering the total downward flux is just the direct beam.
    assert np.allclose(flx[0, :, :, 1].numpy(), analytic, rtol=0, atol=1e-9)
    assert max_err < 1e-9, f"Beer-Lambert check failed: {max_err}"
    print(
        "OK: direct beam reproduces the Beer-Lambert law to machine precision."
    )


if __name__ == "__main__":
    main()
