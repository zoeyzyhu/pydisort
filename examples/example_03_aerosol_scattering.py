#!/usr/bin/env python3
"""Example 3: Multiple scattering by aerosol, and a remote-sensing lookup table.

Scattering is what DISORT exists for, and it is the expensive part of a
radiative transfer calculation: every discrete ordinate is coupled to every
other one through the phase function.  This example builds the forward model
behind a standard satellite aerosol retrieval -- top-of-atmosphere reflectance
as a function of aerosol optical depth and viewing geometry -- for a
Henyey-Greenstein scattering aerosol over a Lambertian surface.

Because a retrieval needs the forward model evaluated on a whole grid of
optical depths, all of them are solved in one batched
:meth:`~pydisort.Disort.forward` call by placing them on the column axis.

Two things differ from the flux-only examples:

* the ``onlyfl`` flag is dropped and ``usrang`` is added, so DISORT returns
  radiances at user-specified viewing angles via
  :meth:`~pydisort.Disort.gather_rad`;
* the beam flux is normalised as ``F_beam = pi / mu0``, which makes the
  reported radiance a bidirectional reflectance -- a Lambertian surface of
  albedo A under a transparent atmosphere then reads back exactly A.

Run with::

    python example_03_aerosol_scattering.py
"""

import numpy as np
import torch

from pydisort import Disort, DisortOptions, scattering_moments

torch.set_default_dtype(torch.float64)

NLYR = 6  # aerosol distributed over 6 layers
NSTR = 16  # 16 streams: enough for a forward-peaked phase function
SSALB = 0.95  # single-scattering albedo of the aerosol
ASYM = 0.70  # Henyey-Greenstein asymmetry parameter
ALBEDO = 0.10  # Lambertian surface reflectance
MU0 = 0.60  # cosine of the solar zenith angle

# The retrieval grid: aerosol optical depths, one per column.
AOD = np.array([0.0, 0.05, 0.1, 0.2, 0.4, 0.8, 1.6, 3.2])

# Viewing directions.  Positive mu is upward-travelling, i.e. what a
# space-borne instrument sees.
VIEW_MU = np.array([0.3, 0.5, 0.7, 1.0])
VIEW_PHI = np.array([0.0, 90.0, 180.0])


def build_solver(ncol: int, onlyfl: bool) -> Disort:
    flags = "usrtau,lamber,quiet,intensity_correction,old_intensity_correction"
    flags += ",onlyfl" if onlyfl else ",usrang"

    op = DisortOptions().flags(flags)
    op.ds().nlyr = NLYR
    op.ds().nstr = NSTR
    op.ds().nmom = NSTR
    op.ds().nphase = NSTR

    # Report results at the top of the atmosphere only.  Note that `user_tau`
    # is shared by every column, so it may not exceed the total optical depth
    # of the *thinnest* column -- tau = 0 is always safe.
    op.user_tau(np.array([0.0]))
    if not onlyfl:
        op.user_mu(VIEW_MU)
        op.user_phi(VIEW_PHI)

    op.accur(0.0)
    op.nwave(1)
    op.ncol(ncol)

    return Disort(op)


def aerosol_properties(aod: np.ndarray, ssalb: float) -> torch.Tensor:
    """Layer optical properties for `len(aod)` columns, shape (1, ncol, nlyr, nprop)."""
    ncol = aod.size
    prop = torch.zeros((1, ncol, NLYR, 2 + NSTR))
    # Spread each column's optical depth evenly over the layers.
    prop[0, :, :, 0] = torch.from_numpy(aod)[:, None] / NLYR
    prop[0, :, :, 1] = ssalb
    # Phase-function moments; identical in every layer and every column.
    prop[0, :, :, 2:] = scattering_moments(NSTR, "henyey-greenstein", ASYM)
    return prop


def boundary_conditions(ncol: int, albedo: float) -> dict:
    zeros_wc = torch.zeros((1, ncol))
    return {
        "umu0": torch.full((ncol,), MU0),
        "phi0": torch.zeros(ncol),
        # pi / mu0 normalisation turns the radiance into a reflectance
        "fbeam": torch.full((1, ncol), np.pi / MU0),
        "albedo": torch.full((1, ncol), albedo),
        "fisot": zeros_wc,
        "fluor": zeros_wc,
    }


def reflectance_table() -> np.ndarray:
    """TOA bidirectional reflectance, shape (naod, nphi, nmu)."""
    ncol = AOD.size
    ds = build_solver(ncol, onlyfl=False)
    ds.forward(
        aerosol_properties(AOD, SSALB), **boundary_conditions(ncol, ALBEDO)
    )
    # gather_rad returns (nwave, ncol, nphi, ntau, numu); ntau == 1 here.
    return ds.gather_rad()[0, :, :, 0, :].numpy()


def conservative_scattering_check() -> float:
    """With omega = 1 and a black surface, no photon may be lost in the
    atmosphere.  Returns the largest relative energy-budget residual."""
    ncol = AOD.size
    op = DisortOptions().flags("onlyfl,lamber,quiet")
    op.ds().nlyr = NLYR
    op.ds().nstr = NSTR
    op.ds().nmom = NSTR
    op.ds().nphase = NSTR
    op.nwave(1)
    op.ncol(ncol)
    ds = Disort(op)

    flx = ds.forward(
        aerosol_properties(AOD, 1.0), **boundary_conditions(ncol, 0.0)
    ).numpy()

    incoming = flx[0, :, 0, 1]  # downward flux at the model top
    reflected = flx[0, :, 0, 0]  # upward flux at the model top
    transmitted = flx[0, :, -1, 1]  # downward flux reaching the black surface
    residual = incoming - reflected - transmitted
    return float(np.abs(residual / incoming).max())


def main() -> None:
    table = reflectance_table()

    print("Top-of-atmosphere bidirectional reflectance")
    print(
        f"aerosol: omega = {SSALB}, g = {ASYM} (Henyey-Greenstein), "
        f"{NSTR} streams"
    )
    print(f"surface: Lambertian, albedo = {ALBEDO};  solar mu0 = {MU0}")

    for j, phi in enumerate(VIEW_PHI):
        print()
        print(f"relative azimuth phi = {phi:g} deg")
        header = "".join(f"{f'mu={m:g}':>10}" for m in VIEW_MU)
        print(f"{'AOD':>6}" + header)
        for i, aod in enumerate(AOD):
            row = "".join(
                f"{table[i, j, k]:>10.4f}" for k in range(VIEW_MU.size)
            )
            print(f"{aod:>6.2f}" + row)

    # The zero-AOD row is the bare Lambertian surface and must return the
    # surface albedo exactly, in every direction.
    bare = table[0]
    print()
    print(
        f"AOD = 0 reflectance: min {bare.min():.6f}, max {bare.max():.6f} "
        f"(surface albedo {ALBEDO})"
    )

    # --- validation -------------------------------------------------------
    print()
    err_bare = float(np.abs(bare - ALBEDO).max())
    print(f"bare-surface check           : max deviation {err_bare:.2e}")
    assert (
        err_bare < 1e-10
    ), f"transparent atmosphere is not neutral: {err_bare}"

    # DISORT handles omega = 1 by perturbing it slightly away from unity, so
    # the conservative-scattering budget closes to about 1e-9 rather than to
    # machine precision.  That residual is the accuracy of the special case,
    # not an error in the setup.
    residual = conservative_scattering_check()
    print(
        f"conservative-scattering check: max relative residual {residual:.2e}"
    )
    assert residual < 1e-7, f"energy is not conserved: {residual}"

    # Scattering aerosol over a dark surface brightens the scene, and the
    # brightening grows with optical depth at every viewing angle.
    nadir = table[:, 0, -1]
    assert np.all(np.diff(nadir) > 0.0), "reflectance is not monotonic in AOD"
    print(
        "OK: bare surface exact, energy conserved, reflectance monotonic in AOD."
    )


if __name__ == "__main__":
    main()
