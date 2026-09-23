#!/usr/bin/env python3
"""Compare pydisort against PythonicDISORT on DISORT Test Problem 9.

PythonicDISORT (Ho 2024) is a pure-Python reimplementation of DISORT; pydisort
wraps the C one. This measures end-to-end solver performance on the same
radiative transfer problem.

The script:

1. builds the identical problem for both, from the shared definition in
   ``testproblem09.py``;
2. checks numerical agreement before reporting timings;
3. includes evaluation of PythonicDISORT's output callables at the requested
   points in the timed calculation;
4. limits PythonicDISORT's native thread pools to one thread;
5. records the full provenance -- CPU, thread counts, and the version of every
   package involved.

Use the recorded settings and versions when comparing runs on your hardware.

Usage
-----
Quick check that everything is wired up correctly (a few seconds)::

    python compare_pythonicdisort.py --verify-only

Default sweep, up to 1000 wavenumbers (a few minutes, dominated by
PythonicDISORT)::

    python compare_pythonicdisort.py

Larger sweep::

    python compare_pythonicdisort.py --nwave 1,10,100,1000 --threads 10

Extending the sweep to 10000 wavenumbers costs about nine minutes on an Apple
M5 Max, and closer to an hour on an M1 Max, essentially all of it
PythonicDISORT, and it does not change the conclusion: the single-core ratio
is flat in nwave and the multi-threaded one has already saturated by 1000. Add
it when you want the asymptote confirmed, not when checking a result.

Requirements::

    pip install pydisort PythonicDISORT threadpoolctl
"""

# Set startup limits before importing numerical libraries, overriding inherited
# settings. Runtime limits below also cover already-loaded BLAS/OpenMP pools.
# The pydisort timing sets its own intra-op thread count with torch.set_num_threads.
import os

for _var in (
    "OPENBLAS_NUM_THREADS",
    "GOTO_NUM_THREADS",
    "BLIS_NUM_THREADS",
    "MKL_NUM_THREADS",
    "VECLIB_MAXIMUM_THREADS",
    "NUMEXPR_NUM_THREADS",
    "OMP_NUM_THREADS",
):
    os.environ[_var] = "1"

import argparse  # noqa: E402
import platform  # noqa: E402
import subprocess  # noqa: E402
import sys  # noqa: E402
import time  # noqa: E402
import warnings  # noqa: E402

try:
    from threadpoolctl import threadpool_limits
except ImportError as exc:
    message = (
        "This benchmark requires threadpoolctl to enforce single-threaded "
        "PythonicDISORT runs. Install it with: python -m pip install threadpoolctl"
    )
    if __name__ == "__main__":
        raise SystemExit(message) from exc
    raise ModuleNotFoundError(message, name="threadpoolctl") from exc

import numpy as np  # noqa: E402
import torch  # noqa: E402

import PythonicDISORT  # noqa: E402

import testproblem09 as tp9  # noqa: E402
from testproblem09 import (  # noqa: E402
    ALBEDO,
    FBEAM,
    FISOT,
    NLYR,
    NSTR,
    PHI0,
    SSALB_SLOPE,
    UMU0,
    USER_MU,
    USER_PHI,
    USER_TAU,
    build_pydisort,
    layer_optical_depth,
    layer_ssalb,
    pydisort_flux,
)

torch.set_default_dtype(torch.float64)


# The problem definition, the pydisort builder and the provenance helpers
# live in testproblem09.py, which compare_cdisort.py imports as well. Two
# benchmark scripts carrying their own copies of the configuration is how a
# published speed-up ratio drifts away from the problem it claims to measure.
DEFAULT_NWAVE = (1, 10, 100, 1000)


# ---------------------------------------------------------------------------
# PythonicDISORT
# ---------------------------------------------------------------------------
def pythonicdisort_call(radiance):
    """One PythonicDISORT solve of the identical problem.

    The two conventions that have to line up:

    * ``tau_arr`` is cumulative optical depth, whereas pydisort takes the
      per-layer thickness;
    * ``Leg_coeffs_all`` includes moment 0 (always 1), whereas pydisort's
      ``scattering_moments`` starts at moment 1.

    Delta-M scaling is not a factor here: the phase function is isotropic, so
    every moment above the zeroth is zero and there is nothing to truncate.
    """
    tau_arr = np.cumsum(layer_optical_depth())
    omega_arr = layer_ssalb()

    legendre = np.zeros((NLYR, NSTR))
    legendre[:, 0] = 1.0  # isotropic

    return PythonicDISORT.pydisort(
        tau_arr,
        omega_arr,
        NSTR,
        legendre,
        mu0=UMU0,
        I0=FBEAM,
        phi0=PHI0,
        b_neg=FISOT,  # isotropic illumination from above
        only_flux=not radiance,
    )


@threadpool_limits.wrap(limits=1)
def pythonicdisort_flux(radiance):
    """Flux at each `user_tau`, shaped like `pydisort_flux`."""
    with warnings.catch_warnings():
        # PythonicDISORT warns that a few eigenvalues come out marginally
        # complex on this problem. It does not affect the fluxes.
        warnings.simplefilter("ignore")
        out = pythonicdisort_call(radiance)

    flux_up, flux_down = out[1], out[2]
    diffuse, direct = flux_down(USER_TAU)
    return np.column_stack([flux_up(USER_TAU), diffuse + direct])


# ---------------------------------------------------------------------------
# Verification
# ---------------------------------------------------------------------------
def verify(radiance, tolerance):
    """Check the two solvers agree. Returns (ok, max_relative_difference).

    The comparison is always on fluxes, in whichever mode is being timed.
    Radiances cannot be compared directly: DISORT reports intensity at
    user-chosen angles, while PythonicDISORT reports it at its own Gauss
    quadrature angles, so lining the two up would need an interpolation whose
    error is not the solver's. Both produce fluxes in both modes, and a solver
    that gets the fluxes right at five optical depths spanning ten orders of
    magnitude is not quietly solving a different problem.
    """
    mine = pydisort_flux(radiance)
    theirs = pythonicdisort_flux(radiance)

    if mine.shape != theirs.shape:
        print(
            f"  shape mismatch: pydisort {mine.shape} vs "
            f"PythonicDISORT {theirs.shape}",
            file=sys.stderr,
        )
        return False, float("inf")

    # Scale by the largest value in the field rather than pointwise: this
    # problem attenuates over ten orders of magnitude, and a relative
    # tolerance on a 1e-9 flux measures rounding, not agreement.
    scale = np.abs(theirs).max()
    error = float(np.abs(mine - theirs).max() / scale)

    mode = "radiance mode" if radiance else "flux mode"
    print(f"Cross-validation on fluxes ({mode}, 1 wavenumber)")
    print(
        f"  {'tau':>8} {'flux up (pyd / PyD)':>46} "
        f"{'flux down (pyd / PyD)':>48}"
    )
    for k, tau in enumerate(USER_TAU):
        print(
            f"  {tau:>8.2f} {mine[k, 0]:>22.12e} {theirs[k, 0]:>22.12e} "
            f"{mine[k, 1]:>23.12e} {theirs[k, 1]:>23.12e}"
        )

    print(
        f"\n  maximum relative difference: {error:.3e} "
        f"(tolerance {tolerance:.0e})"
    )
    return error < tolerance, error


# ---------------------------------------------------------------------------
# Timing
# ---------------------------------------------------------------------------
def time_pydisort(nwave, nthreads, repeat, radiance):
    torch.set_num_threads(nthreads)
    ds, prop, bc = build_pydisort(nwave, radiance)

    ds.forward(prop, **bc)  # warm up allocations and caches

    best = float("inf")
    for _ in range(repeat):
        start = time.perf_counter()
        ds.forward(prop, **bc)
        best = min(best, time.perf_counter() - start)
    return best


def pythonicdisort_solve_and_evaluate(radiance):
    """One solve *plus* evaluation of the outputs at the requested points.

    Evaluate PythonicDISORT's output callables at the requested optical depths
    and azimuths so both timed solvers produce the requested outputs.
    """
    out = pythonicdisort_call(radiance)

    flux_up, flux_down = out[1], out[2]
    diffuse, direct = flux_down(USER_TAU)
    flux_up(USER_TAU)

    if radiance:
        # out[4] is the intensity; only present when only_flux is False.
        out[4](USER_TAU, np.deg2rad(USER_PHI))

    return diffuse, direct


def time_pythonicdisort(nwave, repeat, radiance):
    # Apply limits outside the timed loop, including after a multi-threaded
    # pydisort run has changed a shared OpenMP or BLAS runtime's thread count.
    with threadpool_limits(limits=1), warnings.catch_warnings():
        warnings.simplefilter("ignore")
        pythonicdisort_solve_and_evaluate(radiance)  # warm up

        best = float("inf")
        for _ in range(repeat):
            start = time.perf_counter()
            for _ in range(nwave):
                pythonicdisort_solve_and_evaluate(radiance)
            best = min(best, time.perf_counter() - start)
    return best


# ---------------------------------------------------------------------------
# Provenance (helpers shared with compare_cdisort.py)
# ---------------------------------------------------------------------------
cpu_model = tp9.cpu_model
package_version = tp9.package_version
blas_info = tp9.blas_info


def report_environment(threads, radiance, repeat):
    print("=" * 72)
    print("pydisort vs PythonicDISORT -- DISORT Test Problem 9")
    print("=" * 72)
    print(
        f"problem       : {NSTR} streams, {NLYR} layers, "
        f"{'radiances' if radiance else 'fluxes'} at "
        f"{USER_TAU.size} optical depths"
    )
    print(
        f"                single-scattering albedo "
        f"{layer_ssalb()[0]:.3f} to {layer_ssalb()[-1]:.3f}, "
        f"isotropic phase function"
    )
    print(
        f"                isotropic top illumination fisot = 1/pi, "
        f"black surface"
    )
    print()
    print(f"machine       : {cpu_model()}")
    print(
        f"                {platform.system()} {platform.release()} "
        f"{platform.machine()}, {os.cpu_count()} logical CPUs"
    )
    print(f"python        : {platform.python_version()}")
    print(f"pydisort      : {package_version('pydisort')}")
    print(f"PythonicDISORT: {package_version('PythonicDISORT')}")
    print(f"torch         : {torch.__version__}")
    print(f"numpy         : {np.__version__}")
    with threadpool_limits(limits=1):
        for line in blas_info():
            print(f"Pythonic pools: {line}")
    print(
        f"threads       : PythonicDISORT=1; pydisort single=1, multi={threads}; "
        f"timing best of {repeat}"
    )
    print()


# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--nwave",
        default=",".join(map(str, DEFAULT_NWAVE)),
        help=f"comma-separated wavenumber counts "
        f"(default {','.join(map(str, DEFAULT_NWAVE))})",
    )
    parser.add_argument(
        "--threads",
        type=int,
        default=min(10, os.cpu_count() or 1),
        help="threads for the multi-threaded pydisort run (default: 10 or all)",
    )
    parser.add_argument(
        "--repeat",
        type=int,
        default=1,
        help="timing repeats, fastest reported (default 1; PythonicDISORT is "
        "slow enough that repeats get expensive)",
    )
    parser.add_argument(
        "--mode",
        choices=("radiance", "flux"),
        default="radiance",
        help="what both solvers compute. 'radiance' (default) matches the C "
        "driver for Test 9 and is the more expensive path; 'flux' has "
        "both compute fluxes only. The choice changes the ratio "
        "substantially -- see --compare-modes. The correctness check is "
        "on fluxes either way.",
    )
    parser.add_argument(
        "--compare-modes",
        action="store_true",
        help="also time the other mode at one problem size, to show how much "
        "the reported ratio depends on that choice",
    )
    parser.add_argument(
        "--verify-only",
        action="store_true",
        help="check that the two solvers agree, then exit without timing",
    )
    parser.add_argument(
        "--tolerance",
        type=float,
        default=1e-9,
        help="maximum relative difference accepted by the check (default 1e-9)",
    )
    parser.add_argument(
        "--skip-verify",
        action="store_true",
        help="time without checking agreement first. Not recommended: a "
        "runtime ratio between solvers computing different things is "
        "meaningless.",
    )
    args = parser.parse_args()

    nwave_list = [int(v) for v in args.nwave.split(",")]
    radiance = args.mode == "radiance"
    threads = max(1, args.threads)
    multi_label = f"pydisort ({threads} cores)"

    report_environment(threads, radiance, args.repeat)

    # --- verify before timing -------------------------------------------
    if not args.skip_verify:
        ok, _ = verify(radiance, args.tolerance)
        print()
        if not ok:
            print(
                "FAIL: the two solvers do not agree on this problem.",
                file=sys.stderr,
            )
            print(
                "Timings would be meaningless; stopping here.", file=sys.stderr
            )
            return 1
        print(
            "OK: both solvers agree. Timings below are a like-for-like "
            "comparison."
        )
        print()
    else:
        print("WARNING: agreement check skipped (--skip-verify).\n")

    if args.verify_only:
        return 0

    # --- time -------------------------------------------------------------
    # PythonicDISORT dominates the runtime and scales linearly in nwave, so a
    # single solve projects the total closely enough to warn on. Without this
    # a --nwave 10000 sweep looks like a hung process for several minutes.
    with threadpool_limits(limits=1), warnings.catch_warnings():
        warnings.simplefilter("ignore")
        start = time.perf_counter()
        pythonicdisort_solve_and_evaluate(radiance)
        per_solve = time.perf_counter() - start

    projected = per_solve * sum(nwave_list) * args.repeat
    if projected > 120.0:
        print(
            f"NOTE: PythonicDISORT takes {per_solve * 1e3:.0f} ms per solve "
            f"here, so this sweep will take about "
            f"{projected / 60.0:.0f} minutes."
        )
        print(
            "      Most of that is PythonicDISORT itself. Drop the largest "
            "--nwave to shorten it;"
        )
        print(
            "      the pydisort/PythonicDISORT ratio is flat in nwave on one "
            "core and saturates"
        )
        print("      on several, so the small sizes carry the same result.")
        print()

    print(f"{'nwave':>8} {'solver':>22} {'seconds':>12}")
    print("-" * 44)

    rows = []

    def record(nwave, solver, seconds):
        rows.append({"nwave": nwave, "solver": solver, "seconds": seconds})
        print(f"{nwave:>8} {solver:>22} {seconds:>12.6f}", flush=True)

    for nwave in nwave_list:
        record(
            nwave,
            "PythonicDISORT",
            time_pythonicdisort(nwave, args.repeat, radiance),
        )
        record(
            nwave,
            "pydisort (1 core)",
            time_pydisort(nwave, 1, args.repeat, radiance),
        )
        if threads > 1:
            record(
                nwave,
                multi_label,
                time_pydisort(nwave, threads, args.repeat, radiance),
            )

    # --- summarise --------------------------------------------------------
    by = {}
    for r in rows:
        by.setdefault(r["nwave"], {})[r["solver"]] = r["seconds"]

    print()
    print("Speed-up over PythonicDISORT")
    print(
        f"{'nwave':>8} {'vs 1 core':>14} {'vs ' + str(threads) + ' cores':>14}"
    )
    for nwave in sorted(by):
        v = by[nwave]
        single = v["PythonicDISORT"] / v["pydisort (1 core)"]
        cell = f"{single:>13.1f}x"
        if multi_label in v:
            cell += f" {v['PythonicDISORT'] / v[multi_label]:>13.1f}x"
        print(f"{nwave:>8} {cell}")

    print()
    print("Results correspond to the workload, versions and hardware above.")

    if args.compare_modes:
        other = "flux" if radiance else "radiance"
        n = min(nwave_list)
        print()
        print(f"Mode sensitivity, measured at nwave = {n}")
        print(
            f"{'mode':>10} {'PythonicDISORT':>16} {'pydisort 1c':>13} "
            f"{'ratio':>9}"
        )
        for label, is_rad in ((args.mode, radiance), (other, not radiance)):
            p = time_pythonicdisort(n, args.repeat, is_rad)
            d = time_pydisort(n, 1, args.repeat, is_rad)
            print(f"{label:>10} {p:>16.6f} {d:>13.6f} {p / d:>8.1f}x")
        print()
        print("only_flux=True lets PythonicDISORT skip every azimuthal")
        print("Fourier mode above m=0, and the flags do the equivalent for")
        print("pydisort -- but the two save different fractions of their")
        print("runtime, so the reported ratio depends strongly on which mode")
        print("is timed. Any speed-up figure quoted from this benchmark has")
        print("to name the mode.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
