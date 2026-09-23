"""DISORT Test Problem 9, case 1 -- the single source of truth for benchmarks.

Both `compare_cdisort.py` and `compare_pythonicdisort.py` import this module so
that the problem being timed is defined exactly once. Two benchmark scripts
carrying their own copies of the configuration is precisely how a published
speed-up ratio drifts away from the problem it claims to measure.

The configuration is transcribed from ``tests/cdisort213/test_cdisort_09.c``,
which in turn generalizes case 9a of ``disort_test09()`` in
``tests/cdisort213/test_cdisort.c``. At ``NLYR = 6`` and ``SSALB_SLOPE = 0.05``
the two coincide exactly, and the fluxes reproduce the published reference
values in that file (``good.rad[*].rfldn`` / ``.flup``).

The benchmark configuration is 32 streams and 100 layers, with the workload
scaled by repeating the solve over wavenumbers.

Caveat worth knowing before quoting anything from these scripts
---------------------------------------------------------------
``USER_TAU`` ends at 21.0, which is the *total* optical depth only when
``NLYR = 6``. At ``NLYR = 100`` the column is 6*(1+2+...+100)/100 = 303 optical
depths deep, so tau = 21 sits about 7% of the way down and the deepest output
level is no longer the lower boundary. This is inherited from the C driver, it
is identical for every implementation being compared, and it therefore does not
bias the comparison -- but the benchmark configuration is not literally "Test
Problem 9 with more layers".
"""

import os
import platform
import subprocess
import sys
from importlib import metadata

import numpy as np
import torch

# ---------------------------------------------------------------------------
# Problem definition
# ---------------------------------------------------------------------------
NSTR = 32  # discrete-ordinate streams
NLYR = 100  # atmospheric layers
SSALB_SLOPE = 0.003  # single-scattering albedo increment per layer

USER_TAU = np.array([0.0, 1.05, 2.1, 6.0, 21.0])  # output optical depths
USER_MU = np.array([-1.0, -0.2, 0.2, 1.0])  # output polar angles
USER_PHI = np.array([60.0])  # output azimuth [degrees]

UMU0 = 0.5  # cosine of the beam zenith angle
PHI0 = 0.0
FBEAM = 0.0  # no collimated beam in this test
FISOT = 1.0 / np.pi  # isotropic illumination from above
ALBEDO = 0.0  # black lower boundary


def layer_optical_depth(nlyr=NLYR):
    """Per-layer optical thickness, dtauc(lc) = lc / nlyr * 6."""
    return np.arange(1, nlyr + 1) / nlyr * 6.0


def layer_ssalb(nlyr=NLYR, slope=SSALB_SLOPE):
    """Per-layer single-scattering albedo, 0.6 + lc * slope.

    Runs from 0.603 at the top to 0.900 at the bottom, i.e. a strongly
    scattering atmosphere -- which is the point of the test, since scattering
    is what couples the streams and makes DISORT expensive. The slope is
    chosen so the bottom layer reaches 0.9 for any nlyr (0.6 + 6*0.05 =
    0.6 + 100*0.003), which is what lets the problem scale without changing
    its character.
    """
    return 0.6 + np.arange(1, nlyr + 1) * slope


# ---------------------------------------------------------------------------
# pydisort
# ---------------------------------------------------------------------------
def build_pydisort(nwave, radiance, nstr=NSTR, nlyr=NLYR, slope=SSALB_SLOPE):
    """Configure a solver and its inputs for `nwave` wavenumbers.

    Returns (solver, prop, bc). Construction is deliberately kept out of the
    timed region by the callers, which is fair only because pydisort's setup
    is ~0.3% of its solve at these sizes -- and because the C baseline is
    offered in a matching "allocate once" mode. See `compare_cdisort.py`.
    """
    from pydisort import Disort, DisortOptions, scattering_moments

    flags = "usrtau,lamber,quiet,intensity_correction,old_intensity_correction"
    flags += ",usrang" if radiance else ",onlyfl"

    op = DisortOptions().header("Test Problem 9, Case 1").flags(flags)
    op.ds().nlyr = nlyr
    op.ds().nstr = nstr
    op.ds().nmom = nstr
    op.ds().nphase = nstr
    op.user_tau(USER_TAU)
    if radiance:
        op.user_mu(USER_MU)
        op.user_phi(USER_PHI)
    op.accur(0.0)
    op.ncol(1)
    op.nwave(nwave)

    ds = Disort(op)

    prop = torch.zeros((nwave, 1, nlyr, 2 + nstr), dtype=torch.float64)
    prop[:, :, :, 0] = torch.from_numpy(layer_optical_depth(nlyr))
    prop[:, :, :, 1] = torch.from_numpy(layer_ssalb(nlyr, slope))
    prop[:, :, :, 2:] = scattering_moments(nstr, "isotropic")

    bc = {
        "umu0": torch.tensor([UMU0], dtype=torch.float64),
        "phi0": torch.tensor([PHI0], dtype=torch.float64),
        "fbeam": torch.full((nwave, 1), FBEAM, dtype=torch.float64),
        "fisot": torch.full((nwave, 1), FISOT, dtype=torch.float64),
        "fluor": torch.zeros((nwave, 1), dtype=torch.float64),
        "albedo": torch.full((nwave, 1), ALBEDO, dtype=torch.float64),
    }
    return ds, prop, bc


def pydisort_flux(radiance, **kwargs):
    """Flux at each `USER_TAU`, shape (ntau, 2) as [upward, downward].

    `forward` honours `user_tau` and returns exactly those levels, whereas
    `gather_flx` reports the full internal level grid -- so this return value
    is what lines up with the other implementations. The downward column is
    direct + diffuse (rfldir + rfldn), matching FLX(...) in
    src/disort_impl.h. Fluxes come back in both radiance and flux mode, which
    is what makes them usable as the common cross-check.
    """
    ds, prop, bc = build_pydisort(1, radiance, **kwargs)
    return ds.forward(prop, **bc)[0, 0].numpy()


# ---------------------------------------------------------------------------
# Provenance
# ---------------------------------------------------------------------------
def cpu_model():
    """Best-effort CPU name across Linux and macOS."""
    try:
        if sys.platform.startswith("linux"):
            with open("/proc/cpuinfo") as handle:
                for line in handle:
                    if line.startswith("model name"):
                        return line.split(":", 1)[1].strip()
        elif sys.platform == "darwin":
            return subprocess.check_output(
                ["sysctl", "-n", "machdep.cpu.brand_string"], text=True
            ).strip()
    except Exception:
        pass
    return platform.processor() or "unknown"


def package_version(name):
    try:
        return metadata.version(name)
    except metadata.PackageNotFoundError:
        return "unknown"


def blas_info():
    """Report the BLAS backend and its real thread count, if discoverable."""
    try:
        import threadpoolctl

        return [
            f"{p['internal_api']} ({p.get('prefix', '?')}) "
            f"num_threads={p['num_threads']}"
            for p in threadpoolctl.threadpool_info()
        ]
    except ImportError:
        pinned = os.environ.get("OPENBLAS_NUM_THREADS", "unset")
        return [
            f"OPENBLAS_NUM_THREADS={pinned} "
            "(pip install threadpoolctl to confirm the real count)"
        ]
