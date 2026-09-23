#!/usr/bin/env python3
"""Compare pydisort against cdisort on DISORT Test Problem 9.

pydisort wraps cdisort, so the question this answers is what the wrapper
costs: whether the C++/pybind11/tensor layer adds measurable overhead on a
single core, and how much batching over wavenumbers buys on several.

A runtime ratio between two programs only means something if the two solve the
same problem and the timing is fair in both directions. This script therefore:

1. builds the C baseline itself, from `bench_cdisort.cpp`, against the
   repository's own bundled cdisort headers and optimized compiler flags
   -- no cmake or libtorch needed for the baseline;
2. **verifies that the two implementations agree numerically before reporting
   any timing**, and refuses to time them if they do not;
3. times the C solve loop *inside* the C program, so process startup is not
   charged to cdisort (at small nwave that alone is worth a factor of two);
4. offers the C baseline in two allocation modes, because
   `tests/cdisort213/test_cdisort_09.c` allocates its disort_state on every
   iteration while pydisort allocates once for the whole batch -- see
   `--alloc`;
5. records the CPU, compiler, thread counts and versions, because these ratios
   are hardware- and build-dependent and are not reproducible without them.

Usage
-----
Check that both implementations agree, then exit (a few seconds)::

    python compare_cdisort.py --verify-only

Default sweep::

    python compare_cdisort.py

Larger sweep, more threads, best of three::

    python compare_cdisort.py --nwave 1,10,100,1000,10000 --threads 10 \\
        --repeat 3

Show how much the C baseline's per-iteration allocation costs::

    python compare_cdisort.py --alloc both

Requirements: a C++17 compiler, and `pip install pydisort`.
`threadpoolctl` is optional and only sharpens the BLAS report.
"""

# Thread limits are read by the BLAS backend when numpy first loads it, so they
# have to be set before numpy is imported. OMP_NUM_THREADS is deliberately left
# alone: PyTorch uses it for its own intra-op pool, and pinning it here would
# silently cap the multi-threaded pydisort run we are trying to measure.
# cdisort itself uses bundled LINPACK routines, not BLAS, so it is unaffected
# either way. Preserve any thread settings already supplied by the caller.
# compare_pythonicdisort.py instead enforces one thread for its Python baseline.
import os

for _var in (
    "OPENBLAS_NUM_THREADS",
    "MKL_NUM_THREADS",
    "VECLIB_MAXIMUM_THREADS",
    "NUMEXPR_NUM_THREADS",
):
    os.environ.setdefault(_var, "1")

import argparse  # noqa: E402
import platform  # noqa: E402
import shutil  # noqa: E402
import subprocess  # noqa: E402
import sys  # noqa: E402
import time  # noqa: E402
from pathlib import Path  # noqa: E402

import numpy as np  # noqa: E402
import torch  # noqa: E402

import testproblem09 as tp9  # noqa: E402

torch.set_default_dtype(torch.float64)

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
SOURCE = HERE / "bench_cdisort.cpp"

# Matches cmake/compilers.cmake for a Release build with clang/gcc, so the
# baseline is compiled the way the project ships it rather than at -O0.
CXXFLAGS = [
    "-std=c++17",
    "-O3",
    "-funroll-loops",
    "-fstrict-aliasing",
    "-DNDEBUG",
]

DEFAULT_NWAVE = (1, 10, 100, 1000)


# ---------------------------------------------------------------------------
# Building the C baseline
# ---------------------------------------------------------------------------
def find_compiler():
    for name in (os.environ.get("CXX"), "c++", "clang++", "g++"):
        if name and shutil.which(name):
            return shutil.which(name)
    return None


def compiler_version(cxx):
    try:
        out = subprocess.check_output(
            [cxx, "--version"], text=True, stderr=subprocess.STDOUT
        )
        return out.splitlines()[0].strip()
    except Exception:
        return "unknown"


def build_cdisort(outdir, force=False):
    """Compile bench_cdisort.cpp; returns (path, compiler, version).

    cdisort is header-only (see cdisort213/CMakeLists.txt -- it is an INTERFACE
    target), so this is a single translation unit with the repository root on
    the include path and libm. Nothing else is needed.
    """
    if not SOURCE.exists():
        raise SystemExit(f"missing C source: {SOURCE}")

    cxx = find_compiler()
    if cxx is None:
        raise SystemExit(
            "no C++ compiler found (tried $CXX, c++, clang++, g++). "
            "The cdisort baseline cannot be built."
        )

    outdir.mkdir(parents=True, exist_ok=True)
    binary = outdir / "bench_cdisort"

    fresh = (
        binary.exists()
        and binary.stat().st_mtime >= SOURCE.stat().st_mtime
        and not force
    )
    if not fresh:
        cmd = [
            cxx,
            *CXXFLAGS,
            f"-I{REPO}",
            str(SOURCE),
            "-o",
            str(binary),
            "-lm",
        ]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(" ".join(cmd), file=sys.stderr)
            print(result.stdout, file=sys.stderr)
            print(result.stderr, file=sys.stderr)
            raise SystemExit("failed to build the cdisort baseline")

    return binary, cxx, compiler_version(cxx)


def run_cdisort(binary, args):
    result = subprocess.run(
        [str(binary), *args], capture_output=True, text=True
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        print(result.stderr, file=sys.stderr)
        raise SystemExit(f"cdisort baseline failed: {' '.join(args)}")
    return result.stdout


def cdisort_flux(binary, radiance):
    """Flux at each USER_TAU, shaped like `tp9.pydisort_flux`."""
    out = run_cdisort(
        binary,
        [
            "--nstr",
            str(tp9.NSTR),
            "--nlyr",
            str(tp9.NLYR),
            "--ssalb",
            str(tp9.SSALB_SLOPE),
            "--mode",
            "radiance" if radiance else "flux",
            "--verify",
        ],
    )
    rows = [
        [float(v) for v in line.split()]
        for line in out.splitlines()
        if line and not line.startswith("#")
    ]
    data = np.array(rows)
    if data.shape != (tp9.USER_TAU.size, 3):
        raise SystemExit(f"unexpected --verify output shape {data.shape}")
    if not np.allclose(data[:, 0], tp9.USER_TAU, rtol=0, atol=1e-12):
        raise SystemExit(
            "the C baseline reported a different user_tau grid than "
            "testproblem09.py -- the two are out of sync"
        )
    return data[:, 1:]


def time_cdisort(binary, nwave, repeat, radiance, alloc):
    out = run_cdisort(
        binary,
        [
            "--nstr",
            str(tp9.NSTR),
            "--nlyr",
            str(tp9.NLYR),
            "--ssalb",
            str(tp9.SSALB_SLOPE),
            "--mode",
            "radiance" if radiance else "flux",
            "--alloc",
            alloc,
            "--nwave",
            str(nwave),
            "--repeat",
            str(repeat),
        ],
    )
    for line in out.splitlines():
        if line.startswith("SECONDS "):
            return float(line.split()[1])
    raise SystemExit("the C baseline did not report a timing line")


# ---------------------------------------------------------------------------
# pydisort
# ---------------------------------------------------------------------------
def time_pydisort(nwave, nthreads, repeat, radiance):
    torch.set_num_threads(nthreads)
    if torch.get_num_threads() != nthreads:
        print(
            f"  warning: asked for {nthreads} torch threads, got "
            f"{torch.get_num_threads()}",
            file=sys.stderr,
        )

    ds, prop, bc = tp9.build_pydisort(nwave, radiance)
    ds.forward(prop, **bc)  # warm up allocations and caches

    best = float("inf")
    for _ in range(repeat):
        start = time.perf_counter()
        ds.forward(prop, **bc)
        best = min(best, time.perf_counter() - start)
    return best


# ---------------------------------------------------------------------------
# Verification
# ---------------------------------------------------------------------------
def verify(binary, radiance, tolerance):
    """Check the two implementations agree. Returns (ok, max_rel_difference).

    The comparison is on fluxes at the five output depths. Radiances are not
    compared here because the C driver and pydisort report them through
    different paths; fluxes are produced identically by both in either mode,
    and agreement across five depths spanning ten orders of magnitude is not
    something two different problems produce by accident.
    """
    mine = tp9.pydisort_flux(radiance)
    theirs = cdisort_flux(binary, radiance)

    if mine.shape != theirs.shape:
        print(
            f"  shape mismatch: pydisort {mine.shape} vs "
            f"cdisort {theirs.shape}",
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
        f"  {'tau':>8} {'flux up (pyd / cdisort)':>46} "
        f"{'flux down (pyd / cdisort)':>48}"
    )
    for k, tau in enumerate(tp9.USER_TAU):
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
# Provenance
# ---------------------------------------------------------------------------
def report_environment(threads, radiance, repeat, alloc, cxx, cxxver):
    print("=" * 72)
    print("pydisort vs cdisort -- DISORT Test Problem 9")
    print("=" * 72)
    print(
        f"problem       : {tp9.NSTR} streams, {tp9.NLYR} layers, "
        f"{'radiances' if radiance else 'fluxes'} at "
        f"{tp9.USER_TAU.size} optical depths"
    )
    print(
        f"                single-scattering albedo "
        f"{tp9.layer_ssalb()[0]:.3f} to {tp9.layer_ssalb()[-1]:.3f}, "
        f"isotropic phase function"
    )
    print(
        f"                isotropic top illumination fisot = 1/pi, "
        f"black surface"
    )
    print()
    print(f"machine       : {tp9.cpu_model()}")
    print(
        f"                {platform.system()} {platform.release()} "
        f"{platform.machine()}, {os.cpu_count()} logical CPUs"
    )
    print(f"python        : {platform.python_version()}")
    print(f"pydisort      : {tp9.package_version('pydisort')}")
    print(f"torch         : {torch.__version__}")
    print(f"numpy         : {np.__version__}")
    print(f"compiler      : {cxxver}")
    print(f"                {cxx} {' '.join(CXXFLAGS)}")
    for line in tp9.blas_info():
        print(f"BLAS          : {line}")
    print(
        f"threads       : pydisort single=1, multi={threads}; "
        f"cdisort single-threaded; timing best of {repeat}"
    )
    print(f"cdisort alloc : {alloc}")
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
        default=3,
        help="timing repeats, fastest reported (default 3)",
    )
    parser.add_argument(
        "--mode",
        choices=("radiance", "flux"),
        default="radiance",
        help="what both implementations compute. 'radiance' (default) matches "
        "test_cdisort_09.c and is the more expensive path.",
    )
    parser.add_argument(
        "--alloc",
        choices=("hoisted", "percall", "both"),
        default="hoisted",
        help="how the C baseline manages its disort_state. 'hoisted' "
        "(default) allocates once for the whole loop, which is what "
        "pydisort does and is therefore the like-for-like comparison. "
        "'percall' allocates and frees on every iteration, which is what "
        "tests/cdisort213/test_cdisort_09.c does. 'both' reports each.",
    )
    parser.add_argument(
        "--verify-only",
        action="store_true",
        help="check that the two implementations agree, then exit",
    )
    parser.add_argument(
        "--tolerance",
        type=float,
        default=1e-12,
        help="maximum relative difference accepted (default 1e-12; pydisort "
        "calls the same solver, so agreement should be near machine "
        "precision, not merely close)",
    )
    parser.add_argument(
        "--skip-verify",
        action="store_true",
        help="time without checking agreement first. Not recommended.",
    )
    parser.add_argument(
        "--rebuild",
        action="store_true",
        help="force a rebuild of the C driver",
    )
    parser.add_argument(
        "--builddir",
        default=None,
        help="where to put the compiled baseline (default: ./_bench_build)",
    )
    args = parser.parse_args()

    nwave_list = [int(v) for v in args.nwave.split(",")]
    radiance = args.mode == "radiance"
    threads = max(1, args.threads)
    multi_label = f"pydisort ({threads} cores)"

    allocs = ["hoisted", "percall"] if args.alloc == "both" else [args.alloc]
    alloc_labels = [f"cdisort ({a})" for a in allocs]

    builddir = Path(args.builddir) if args.builddir else HERE / "_bench_build"
    binary, cxx, cxxver = build_cdisort(builddir, force=args.rebuild)

    report_environment(
        threads, radiance, args.repeat, ", ".join(allocs), cxx, cxxver
    )

    # --- verify before timing -------------------------------------------
    if not args.skip_verify:
        ok, _ = verify(binary, radiance, args.tolerance)
        print()
        if not ok:
            print(
                "FAIL: pydisort and cdisort do not agree on this problem.",
                file=sys.stderr,
            )
            print(
                "Timings would be meaningless; stopping here.", file=sys.stderr
            )
            return 1
        print(
            "OK: both implementations agree. Timings below are a "
            "like-for-like comparison."
        )
        print()
    else:
        print("WARNING: agreement check skipped (--skip-verify).\n")

    if args.verify_only:
        return 0

    # --- time -------------------------------------------------------------
    print(f"{'nwave':>8} {'implementation':>22} {'seconds':>12}")
    print("-" * 44)

    rows = []

    def record(nwave, solver, seconds):
        rows.append({"nwave": nwave, "solver": solver, "seconds": seconds})
        print(f"{nwave:>8} {solver:>22} {seconds:>12.6f}", flush=True)

    for nwave in nwave_list:
        for alloc, label in zip(allocs, alloc_labels):
            record(
                nwave,
                label,
                time_cdisort(binary, nwave, args.repeat, radiance, alloc),
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

    baseline = alloc_labels[0]
    print()
    print(f"Speed-up over {baseline}")
    print(f"{'nwave':>8} {'1 core':>14} {str(threads) + ' cores':>14}")
    for nwave in sorted(by):
        v = by[nwave]
        cell = f"{v[baseline] / v['pydisort (1 core)']:>13.2f}x"
        if multi_label in v:
            cell += f" {v[baseline] / v[multi_label]:>13.2f}x"
        print(f"{nwave:>8} {cell}")

    if len(alloc_labels) > 1:
        print()
        print("Cost of the C baseline's per-iteration state allocation")
        print(f"{'nwave':>8} {'percall/hoisted':>18}")
        for nwave in sorted(by):
            v = by[nwave]
            if all(lbl in v for lbl in alloc_labels):
                print(
                    f"{nwave:>8} "
                    f"{v['cdisort (percall)'] / v['cdisort (hoisted)']:>17.3f}x"
                )
        print(
            "\ntest_cdisort_09.c allocates per call; pydisort allocates once."
        )
        print("Timing the first against the second without this switch would")
        print("credit pydisort with the difference shown above.")

    print()
    print(
        "A ratio near 1.0 suggests small wrapper overhead for this workload."
    )
    print("Both call c_disort on the same inputs. The multi-threaded")
    print(
        "ratio climbs while the cores fill and then saturates -- it is bounded"
    )
    print("by the thread count, so quote it with the thread count and the CPU")
    print("from the provenance block above.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
