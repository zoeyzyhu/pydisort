#!/usr/bin/env python3
"""Draw the four-implementation scaling figure.

``compare_cdisort.py`` and ``compare_pythonicdisort.py`` each answer one
question and print a table. This script runs both sweeps and draws them on one
pair of axes: runtime against spectral resolution for cdisort, PythonicDISORT,
and pydisort at one and at N threads.

It reuses those two scripts rather than reimplementing them, so there is no
third copy of the timing code and no third copy of the problem definition,
which lives in ``testproblem09.py``. Whatever they measure is what gets
plotted.

The PythonicDISORT series requires PythonicDISORT and threadpoolctl. If either
is missing, the figure is drawn with three series and says so. The
PythonicDISORT calculation dominates the runtime, taking roughly
nine minutes at ``nwave = 10000`` on a current laptop and far longer on older
hardware. ``--max-pythonicdisort`` caps the sizes it is asked for, and the
remaining series still run to the end of the sweep.

Usage::

    python benchmarks/plot_scaling.py
    python benchmarks/plot_scaling.py --nwave 1,10,100,1000,10000 --threads 10
    python benchmarks/plot_scaling.py --max-pythonicdisort 1000

Requires matplotlib::

    pip install matplotlib

To include the PythonicDISORT series::

    pip install PythonicDISORT threadpoolctl
"""

# Thread limits are read by the BLAS backend when numpy first loads it, so they
# have to be set before numpy is imported, and therefore before the two
# compare_* modules are imported. The PythonicDISORT benchmark additionally
# applies runtime limits; pydisort selects its own intra-op thread count in
# each timing call.
import os

for _var in (
    "OPENBLAS_NUM_THREADS",
    "MKL_NUM_THREADS",
    "VECLIB_MAXIMUM_THREADS",
    "NUMEXPR_NUM_THREADS",
):
    os.environ.setdefault(_var, "1")

import argparse  # noqa: E402
import csv  # noqa: E402
import platform  # noqa: E402
import sys  # noqa: E402
import warnings  # noqa: E402
from pathlib import Path  # noqa: E402

import torch  # noqa: E402

import compare_cdisort as cdi  # noqa: E402
import testproblem09 as tp9  # noqa: E402

try:
    import compare_pythonicdisort as pyd

    HAVE_PYTHONICDISORT = True
except ModuleNotFoundError as exc:
    if exc.name not in {"PythonicDISORT", "threadpoolctl"}:
        raise
    HAVE_PYTHONICDISORT = False

DEFAULT_NWAVE = (1, 10, 100, 1000)

CDISORT = "cdisort"
PYTHONICDISORT = "PythonicDISORT"


def single_label(_threads):
    return "pydisort (1 core)"


def multi_label(threads):
    return f"pydisort ({threads} cores)"


def measure(nwave_list, threads, repeat, radiance, max_pyd, builddir):
    """Time every implementation at every size. Returns {label: {nwave: s}}."""
    binary, cxx, cxxver = cdi.build_cdisort(builddir)

    print("Verifying that the implementations agree before timing anything.")
    ok, _ = cdi.verify(binary, radiance, tolerance=1e-12)
    if not ok:
        sys.exit("pydisort and cdisort disagree; the figure would be a lie.")
    if HAVE_PYTHONICDISORT:
        ok, _ = pyd.verify(radiance, tolerance=1e-9)
        if not ok:
            sys.exit("pydisort and PythonicDISORT disagree.")
    print()

    results = {
        CDISORT: {},
        single_label(threads): {},
        multi_label(threads): {},
    }
    if HAVE_PYTHONICDISORT:
        results[PYTHONICDISORT] = {}

    print(f"{'nwave':>8} {'implementation':>22} {'seconds':>12}")
    print("-" * 44)

    def record(nwave, label, seconds):
        results[label][nwave] = seconds
        print(f"{nwave:>8} {label:>22} {seconds:>12.6f}", flush=True)

    for nwave in nwave_list:
        record(
            nwave,
            CDISORT,
            cdi.time_cdisort(binary, nwave, repeat, radiance, "hoisted"),
        )
        record(
            nwave,
            single_label(threads),
            cdi.time_pydisort(nwave, 1, repeat, radiance),
        )
        if threads > 1:
            record(
                nwave,
                multi_label(threads),
                cdi.time_pydisort(nwave, threads, repeat, radiance),
            )
        if HAVE_PYTHONICDISORT and nwave <= max_pyd:
            with warnings.catch_warnings():
                warnings.simplefilter("ignore")
                record(
                    nwave,
                    PYTHONICDISORT,
                    pyd.time_pythonicdisort(nwave, repeat, radiance),
                )

    return results, cxx, cxxver


def draw(results, threads, path, title_note):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    styles = {
        CDISORT: ("gray", "o"),
        PYTHONICDISORT: ("tab:red", "o"),
        single_label(threads): ("tab:blue", "o"),
        multi_label(threads): ("tab:green", "o"),
    }

    fig, ax = plt.subplots(figsize=(10, 6))
    for label, (color, marker) in styles.items():
        series = results.get(label)
        if not series:
            continue
        x = sorted(series)
        ax.plot(
            x,
            [series[n] for n in x],
            marker=marker,
            color=color,
            label=label,
        )

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Number of Wavenumbers")
    ax.set_ylabel("Total Time (s)")
    ax.grid(True, which="both", lw=0.3, alpha=0.5)
    ax.legend(title="Library")
    if title_note:
        ax.set_title(title_note, fontsize=9)

    fig.tight_layout()
    fig.savefig(path, dpi=150)
    print(f"\nfigure written to {path}")


def write_csv(results, path, threads, repeat, radiance, cxx, cxxver):
    with open(path, "w", newline="") as handle:
        writer = csv.writer(handle)
        # Provenance in the file itself, so a stray CSV stays interpretable.
        writer.writerow(["# cpu", tp9.cpu_model()])
        writer.writerow(
            [
                "# platform",
                f"{platform.system()} {platform.release()} "
                f"{platform.machine()}",
            ]
        )
        writer.writerow(["# pydisort", tp9.package_version("pydisort")])
        writer.writerow(
            ["# PythonicDISORT", tp9.package_version("PythonicDISORT")]
        )
        writer.writerow(["# torch", torch.__version__])
        writer.writerow(["# compiler", cxxver])
        writer.writerow(["# cxx", cxx])
        writer.writerow(["# threads_multi", threads])
        writer.writerow(["# repeat", repeat])
        writer.writerow(["# mode", "radiance" if radiance else "flux"])
        writer.writerow(["nwave", "implementation", "seconds"])
        for label, series in results.items():
            for nwave in sorted(series):
                writer.writerow([nwave, label, f"{series[nwave]:.6f}"])
    print(f"data written to {path}")


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
        help="what every implementation computes (default radiance)",
    )
    parser.add_argument(
        "--max-pythonicdisort",
        type=int,
        default=None,
        help="largest nwave to run PythonicDISORT at. It dominates the "
        "runtime; capping it keeps the other three series going to the "
        "end of the sweep without waiting.",
    )
    parser.add_argument(
        "--out",
        default="scaling.png",
        help="figure path (default scaling.png)",
    )
    parser.add_argument(
        "--csv", default="scaling.csv", help="data path (default scaling.csv)"
    )
    parser.add_argument(
        "--builddir",
        default=None,
        help="where to put the compiled C baseline (default ./_bench_build)",
    )
    args = parser.parse_args()

    nwave_list = sorted(int(v) for v in args.nwave.split(","))
    threads = max(1, args.threads)
    radiance = args.mode == "radiance"
    max_pyd = (
        args.max_pythonicdisort
        if args.max_pythonicdisort is not None
        else max(nwave_list)
    )
    builddir = (
        Path(args.builddir)
        if args.builddir
        else Path(__file__).resolve().parent / "_bench_build"
    )

    cdi.report_environment(
        threads, radiance, args.repeat, "hoisted", "see below", ""
    )
    if not HAVE_PYTHONICDISORT:
        print(
            "PythonicDISORT or threadpoolctl is unavailable; drawing three series.\n"
            "    pip install PythonicDISORT threadpoolctl\n"
        )

    results, cxx, cxxver = measure(
        nwave_list, threads, args.repeat, radiance, max_pyd, builddir
    )

    note = (
        f"{tp9.cpu_model()}, {threads} threads, "
        f"pydisort {tp9.package_version('pydisort')}, "
        f"torch {torch.__version__}"
    )
    if HAVE_PYTHONICDISORT:
        note += f", PythonicDISORT {tp9.package_version('PythonicDISORT')}"

    write_csv(results, args.csv, threads, args.repeat, radiance, cxx, cxxver)
    draw(results, threads, args.out, note)

    print(
        "\nThe figure is machine- and version-specific. The caption above it "
        "carries the\nprovenance for exactly that reason: redraw it on the "
        "hardware you intend to quote."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
