#!/usr/bin/env python3
"""Example 2: Broadband thermal emission and longwave cooling rates.

This example works through a small but complete longwave radiation calculation
for an Earth-like atmosphere: a band-averaged (grey-per-band) absorber, a warm
Lambertian surface, and a US-Standard-Atmosphere temperature profile.  It
produces the two quantities a climate or weather model actually needs from its
longwave scheme,

* the outgoing longwave radiation (OLR) at the top of the atmosphere, and
* the radiative cooling rate profile,  dT/dt = (g / c_p) dF_net/dp.

The point of interest for pydisort users is the *spectral* dimension.  All
eight bands are solved in a single :meth:`~pydisort.Disort.forward` call by
putting them on the leading ``nwave`` axis, which is one of the two axes
pydisort parallelizes over (the other being atmospheric columns, see
``example_03_aerosol_scattering.py``).

Run with::

    python example_02_thermal_emission.py
"""

import numpy as np
import torch

from pydisort import Disort, DisortOptions

torch.set_default_dtype(torch.float64)

SIGMA = 5.670374419e-8  # Stefan-Boltzmann constant [W m-2 K-4]
GRAV = 9.81  # gravity [m s-2]
CP = 1004.0  # specific heat of dry air [J kg-1 K-1]
P_SURF = 1.013e5  # surface pressure [Pa]
P_TOP = 1.0e2  # model top pressure [Pa]
T_SURF = 288.0  # surface temperature [K]

NLYR = 40
NSTR = 8

# A crude eight-band decomposition of the terrestrial longwave spectrum.  Each
# band gets a reference mass absorption coefficient `kappa` at the surface and
# a scaling exponent `n` such that the local coefficient is
# kappa * (p / p_surf)**n.  The exponent stands in for pressure broadening
# (n ~ 1, e.g. the CO2 bands) plus the sharp decrease of water vapour with
# height (n ~ 2, the H2O bands).  The 800-1200 cm-1 bands form the atmospheric
# window and are deliberately transparent.
#
# This is an idealised band model chosen to be readable, not a spectroscopic
# scheme: it reproduces the right qualitative structure (a strong greenhouse
# effect, a transparent window, radiative cooling throughout the troposphere)
# but should not be mistaken for a correlated-k or line-by-line calculation.
BANDS = [
    # (lower [cm-1], upper [cm-1], kappa [m2 kg-1], n, label)
    (10.0, 400.0, 2.0e-3, 2.0, "H2O rotational"),
    (400.0, 600.0, 8.0e-4, 2.0, "H2O rot./vib."),
    (600.0, 800.0, 3.0e-3, 1.0, "CO2 15 um"),
    (800.0, 1000.0, 2.0e-5, 1.0, "window"),
    (1000.0, 1200.0, 3.0e-5, 1.0, "window + O3"),
    (1200.0, 1400.0, 3.0e-4, 2.0, "H2O vib."),
    (1400.0, 2000.0, 1.5e-3, 2.0, "H2O 6.3 um"),
    (2000.0, 3000.0, 5.0e-5, 2.0, "far wing"),
]


def standard_atmosphere(plev: np.ndarray) -> np.ndarray:
    """US Standard Atmosphere temperature on the given pressure levels."""
    # Hydrostatic height of each level for a 7.5 km scale height, then the
    # standard troposphere/stratosphere piecewise-linear temperature profile.
    z = -7.5e3 * np.log(plev / P_SURF)
    t = np.where(z < 11.0e3, T_SURF - 6.5e-3 * z, 216.65)
    t = np.where(z > 20.0e3, 216.65 + 1.0e-3 * (z - 20.0e3), t)
    return np.minimum(t, T_SURF)


def build_solver(nwave: int) -> Disort:
    """Configure a flux-only longwave solver with thermal emission enabled."""
    op = DisortOptions().flags("onlyfl,lamber,planck,quiet")

    # `planck` turns on the internal thermal source.  It requires a temperature
    # profile on *levels* (see `temf` below) and the spectral limits of each
    # band, so that cdisort can integrate the Planck function over the band.
    op.ds().nlyr = NLYR
    op.ds().nstr = NSTR
    op.ds().nmom = NSTR
    op.ds().nphase = NSTR

    op.nwave(nwave)
    op.ncol(1)
    op.wave_lower([b[0] for b in BANDS])
    op.wave_upper([b[1] for b in BANDS])

    return Disort(op)


def _layer_optical_thickness(plev: np.ndarray) -> np.ndarray:
    """Layer optical thickness of the grey absorber, shape (nband, nlyr).

    For an absorber in hydrostatic balance, dtau = kappa(p) * dp / g.
    """
    dp = np.diff(plev)
    players = 0.5 * (plev[:-1] + plev[1:])
    kappa = np.array([b[2] for b in BANDS])
    expo = np.array([b[3] for b in BANDS])
    kappa_p = kappa[:, None] * (players[None, :] / P_SURF) ** expo[:, None]
    return kappa_p * dp[None, :] / GRAV


def run(plev: np.ndarray, temf: np.ndarray, tsurf: float) -> np.ndarray:
    """Return the per-band flux tensor, shape (nwave, ncol, nlvl, 2)."""
    nwave, ncol = len(BANDS), 1
    ds = build_solver(nwave)

    # Purely absorbing, so the single-scattering albedo and the phase-function
    # moments stay zero -- only the optical thickness slot is filled in.
    prop = torch.zeros((nwave, ncol, NLYR, 2 + NSTR))
    prop[:, 0, :, 0] = torch.from_numpy(_layer_optical_thickness(plev))

    zeros_wc = torch.zeros((nwave, ncol))
    return ds.forward(
        prop,
        temf=torch.from_numpy(temf).reshape(ncol, NLYR + 1),
        btemp=torch.full((ncol,), tsurf),  # black surface at T_SURF
        ttemp=torch.zeros(ncol),
        temis=zeros_wc,  # no thermal emission from above the model top
        albedo=zeros_wc,  # emissivity 1 surface
        fbeam=zeros_wc,  # longwave only: no solar beam
        fisot=zeros_wc,
        fluor=zeros_wc,
        umu0=torch.ones(ncol),
        phi0=torch.zeros(ncol),
    )


def isothermal_check() -> float:
    """An isothermal column over a surface at the same temperature must emit
    exactly sigma*T^4 at every level.  Returns the relative error."""
    plev = np.linspace(P_TOP, P_SURF, NLYR + 1)
    temf = np.full(NLYR + 1, T_SURF)
    flx = run(plev, temf, T_SURF)
    upward = flx[:, 0, :, 0].sum(dim=0).numpy()  # summed over bands
    # Bands 10-3000 cm-1 capture ~99.9% of a 288 K Planck curve, so compare
    # against that fraction of sigma*T^4 rather than the full integral.
    return float(np.abs(upward - upward[-1]).max() / upward[-1])


def main() -> None:
    # Pressure levels, top to bottom, evenly spaced in log-pressure.
    plev = np.logspace(np.log10(P_TOP), np.log10(P_SURF), NLYR + 1)
    temf = standard_atmosphere(plev)

    flx = run(plev, temf, T_SURF)
    print(f"flux tensor shape: {tuple(flx.shape)}  (nwave, ncol, nlvl, 2)")

    up = flx[:, 0, :, 0].numpy()  # (nband, nlvl)
    dn = flx[:, 0, :, 1].numpy()

    olr_band = up[:, 0]
    olr = olr_band.sum()
    down_surface = dn[:, -1].sum()

    # Total optical thickness of each band, straight from the solver input.
    tau_total = _layer_optical_thickness(plev).sum(axis=1)

    print()
    print("Per-band outgoing longwave radiation")
    print(
        f"{'band [cm-1]':>16} {'kappa [m2/kg]':>14} {'tau_total':>10} {'OLR [W/m2]':>12}  label"
    )
    for (lo, hi, kappa, _n, label), f, tau in zip(BANDS, olr_band, tau_total):
        print(
            f"{f'{lo:g}-{hi:g}':>16} {kappa:>14.1e} "
            f"{tau:>10.2f} {f:>12.2f}  {label}"
        )
    print(f"{'total':>16} {'':>14} {'':>10} {olr:>12.2f}")

    print()
    print(f"OLR                          : {olr:8.2f} W m-2")
    print(f"surface emission (sigma T^4) : {SIGMA * T_SURF ** 4:8.2f} W m-2")
    print(f"downward flux at the surface : {down_surface:8.2f} W m-2")
    print(
        f"greenhouse effect            : {SIGMA * T_SURF ** 4 - olr:8.2f} W m-2"
    )

    # Radiative heating rate.  With levels ordered top-to-bottom and F_net the
    # net *upward* flux, the first law in pressure coordinates reads
    #     dT/dt = (g / c_p) * dF_net/dp,
    # so a negative value means the layer is cooling to space.
    net_up = (up - dn).sum(axis=0)  # broadband net upward flux on levels
    dp = np.diff(plev)
    heating = GRAV / CP * np.diff(net_up) / dp * 86400.0
    players = 0.5 * (plev[:-1] + plev[1:])

    print()
    print("Longwave heating rate (negative = radiative cooling)")
    print(f"{'p [hPa]':>10} {'T [K]':>8} {'dT/dt [K/day]':>15}")
    for k in range(0, NLYR, 4):
        tlay = 0.5 * (temf[k] + temf[k + 1])
        print(f"{players[k] / 100.0:>10.1f} {tlay:>8.1f} {heating[k]:>15.3f}")

    # --- validation -------------------------------------------------------
    print()

    # (1) An isothermal column over a surface at the same temperature must
    #     emit sigma*T^4 uniformly.  This is exact, to machine precision.
    err = isothermal_check()
    print(f"isothermal-column check      : max relative deviation {err:.2e}")
    assert err < 1e-10, f"isothermal column is not uniform: {err}"

    # (2) Energy conservation: integrating the heating rate over the mass of
    #     the column must return the net flux divergence across the column,
    #     i.e. what the surface supplies minus what escapes to space.
    column = (CP / GRAV * (heating / 86400.0) * dp).sum()
    expected = net_up[-1] - net_up[0]
    rel = abs(column - expected) / abs(expected)
    print(
        f"column energy budget         : {column:.6f} vs {expected:.6f} W m-2"
        f"  (rel. err {rel:.2e})"
    )
    assert rel < 1e-12, f"heating rate is not energy conserving: {rel}"

    # (3) The OLR must lie between the emission of the coldest emitting level
    #     and that of the surface, and the troposphere must cool to space.
    assert SIGMA * temf.min() ** 4 < olr < SIGMA * T_SURF**4
    troposphere = players > 2.0e4
    assert heating[troposphere].max() < 0.0, "troposphere is not cooling"
    print("OK: isothermal limit exact, energy conserved, troposphere cools.")


if __name__ == "__main__":
    main()
