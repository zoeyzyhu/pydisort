"""Shared harness for the published DISORT reference problems.

Every module in this directory solves one of the test problems distributed
with DISORT and compares the result against the values published with it. They
differ only in their inputs, so the solver call lives here once, exposed as the
``solve`` fixture, and each module contributes a table of cases.

The reference values themselves are transcribed from
``tests/cdisort213/test_cdisort.c``, which is the C driver those problems ship
in. Keeping the comparison at that level means a discrepancy can be localised:
the same problem is checked at the C level by ``test_cdisort.release``, so if
the C driver passes and the Python one does not, the fault is in the bindings.
"""

from dataclasses import dataclass, field

import numpy as np
import pytest
import torch

from pydisort import Disort, DisortOptions, scattering_moments

# DISORT reference values are published to six significant figures, so there is
# no point demanding more. The absolute floor matters because several problems
# attenuate over ten orders of magnitude and a relative tolerance on a 1e-16
# flux would be measuring rounding rather than physics.
RTOL = 1e-4
ATOL = 1e-6


@dataclass
class Layer:
    """Optical properties of one computational layer.

    `phase` names one of the analytic phase functions `scattering_moments`
    knows how to build. `moments` overrides it with an explicit Legendre
    series starting at moment 1, which is how Test Problem 9b supplies a
    tabulated phase function that has no closed form.
    """

    dtau: float
    ssalb: float
    phase: str = "isotropic"
    gg: float = 0.0
    moments: list[float] | None = None


@dataclass
class Problem:
    """One reference case: the medium, the illumination and the output grid."""

    label: str
    layers: list[Layer]
    user_tau: list[float]
    user_mu: list[float] = field(default_factory=lambda: [1.0])
    user_phi: list[float] = field(default_factory=lambda: [0.0])
    nstr: int = 16
    nmom: int | None = None  # defaults to nstr

    # Illumination. fbeam is the beam flux, fisot uniform diffuse light on the
    # top boundary, fluor an isotropic source at the bottom.
    umu0: float = 1.0
    phi0: float = 0.0
    fbeam: float = 0.0
    fisot: float = 0.0
    fluor: float = 0.0
    albedo: float = 0.0

    # Thermal emission. Setting `temper` switches the planck flag on; it holds
    # nlyr + 1 level temperatures, top boundary first.
    temper: list[float] | None = None
    btemp: float = 0.0
    ttemp: float = 0.0
    temis: float = 0.0
    wavenumber: tuple[float, float] = (0.0, 1.0)

    # usrang=False reports intensities at DISORT's own quadrature angles
    # instead of at user_mu; the fluxes are unaffected, which is what
    # test_problem_10 checks.
    usrang: bool = True

    @property
    def planck(self) -> bool:
        return self.temper is not None


@dataclass
class Case:
    """A reference problem together with the values published for it.

    `flux` is what forward() returns, [upward, downward] at each output depth.
    `detail` is the eight-component cdisort breakdown from gather_flx(), and
    `radiance` the intensities from gather_rad(). The last two are optional:
    some problems publish only fluxes.
    """

    problem: Problem
    flux: list
    detail: list | None = None
    radiance: list | None = None


def build(problem: Problem, ncol: int = 1, nwave: int = 1):
    """Configure a solver and its inputs for `problem`."""
    nstr = problem.nstr
    nmom = problem.nmom or nstr
    nlyr = len(problem.layers)

    flags = ["usrtau", "lamber", "quiet"]
    flags += ["intensity_correction", "old_intensity_correction"]
    if problem.usrang:
        flags.append("usrang")
    if problem.planck:
        flags.append("planck")

    op = DisortOptions().header(problem.label).flags(",".join(flags))
    op.ds().nlyr = nlyr
    op.ds().nstr = nstr
    op.ds().nmom = nmom
    op.ds().nphase = nstr
    op.user_tau(np.array(problem.user_tau, dtype=float))
    op.user_mu(np.array(problem.user_mu, dtype=float))
    op.user_phi(np.array(problem.user_phi, dtype=float))
    op.accur(0.0)
    op.ncol(ncol)
    op.nwave(nwave)
    if problem.planck:
        lo, hi = problem.wavenumber
        op.wave_lower(np.full(nwave, lo))
        op.wave_upper(np.full(nwave, hi))

    ds = Disort(op)

    # cdisort is a double-precision solver; building the inputs at torch's
    # global default would silently compare float32 against six-figure
    # reference values.
    prop = torch.zeros((nwave, ncol, nlyr, 2 + nmom), dtype=torch.float64)
    for i, layer in enumerate(problem.layers):
        prop[:, :, i, 0] = layer.dtau
        prop[:, :, i, 1] = layer.ssalb
        if layer.moments is None:
            prop[:, :, i, 2:] = scattering_moments(nmom, layer.phase, layer.gg)
        else:
            series = torch.zeros(nmom, dtype=torch.float64)
            series[: len(layer.moments)] = torch.tensor(
                layer.moments, dtype=torch.float64
            )
            prop[:, :, i, 2:] = series

    def per_column(value):
        return torch.full((ncol,), float(value), dtype=torch.float64)

    def per_wave_column(value):
        return torch.full((nwave, ncol), float(value), dtype=torch.float64)

    bc = {
        "umu0": per_column(problem.umu0),
        "phi0": per_column(problem.phi0),
        "fbeam": per_wave_column(problem.fbeam),
        "fisot": per_wave_column(problem.fisot),
        "fluor": per_wave_column(problem.fluor),
        "albedo": per_wave_column(problem.albedo),
    }

    temf = None
    if problem.planck:
        bc["btemp"] = per_column(problem.btemp)
        bc["ttemp"] = per_column(problem.ttemp)
        bc["temis"] = per_wave_column(problem.temis)
        temf = torch.tensor(
            [problem.temper] * ncol, dtype=torch.float64
        ).reshape(ncol, nlyr + 1)

    return ds, prop, bc, temf


@pytest.fixture(scope="session")
def solve():
    """Solve a `Problem`; returns (solver, fluxes).

    The returned flux tensor is (nwave, ncol, ntau, 2) as [upward, downward],
    with downward being direct plus diffuse. Use ``solver.gather_flx()`` for
    the full eight-component breakdown and ``solver.gather_rad()`` for
    radiances.
    """

    def _solve(problem: Problem, ncol: int = 1, nwave: int = 1):
        ds, prop, bc, temf = build(problem, ncol=ncol, nwave=nwave)
        flx = ds.forward(prop, temf=temf, **bc)
        return ds, flx

    return _solve
