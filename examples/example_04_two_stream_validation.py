#!/usr/bin/env python3
"""Example 4: Validating a fast two-stream solver (real-world case).

Almost every climate model, weather model and operational retrieval uses a
*two-stream* radiative transfer solver, because two streams are cheap enough to
call millions of times. The price is accuracy, and the only way to know what
that price is, is to compare against a trusted multi-stream reference. DISORT
is that reference, which is why the official DISORT flux-test problems are the
standard yardstick.

This real-world analysis demonstrates the reference-comparison workflow used
to assess fast radiative-transfer approximations. Projects such as ``py2sess``
(https://github.com/happysky19/py2sess) motivate this use of DISORT as a
reference. Here the calculations use pydisort at different stream counts;
the example does not run or measure an external two-stream implementation.

This example reproduces that workflow end to end:

1. **Reproduce the published DISORT benchmark fluxes.** Twelve official
   flux-test cases (Test 1 isotropic, Test 2 Rayleigh, Test 3
   Henyey-Greenstein) are solved with 16 streams and compared against the
   published reference values. This validates the installation against an
   external source rather than against itself.
2. **Measure the two-stream error.** The same twelve cases are re-solved with
   ``nstr = 2`` and ``nmom = 2``. The input retains phase-function moments of
   orders 1 and 2 plus the implicit zeroth moment; the second-order moment
   can affect delta-M scaling. The results describe this DISORT approximation,
   not every two-stream closure.
3. **Answer "how many streams do I need?"** by sweeping the stream count and
   comparing with the published fluxes. This example does not time the solves.

Each stream count solves all twelve cases in a single batched
:meth:`~pydisort.Disort.forward` call, with the cases laid out along the column
axis.

Run with::

    python example_04_two_stream_validation.py
    python example_04_two_stream_validation.py --plot figure.png
"""

import argparse
import contextlib
import os
import sys
import textwrap

import numpy as np
import torch

from pydisort import Disort, DisortOptions, scattering_moments

torch.set_default_dtype(torch.float64)


def paragraph(text: str) -> None:
    """Print a blank line and then `text`, re-wrapped to 79 columns."""
    print()
    print(textwrap.fill(" ".join(text.split()), width=79))


@contextlib.contextmanager
def quiet_backend():
    """Silence writes the C solver makes straight to the output descriptors.

    DISORT emits `2 streams not recommended` from its input checker whenever
    nstr = 2, which is exactly what this example asks it to do, once per case
    per call. The message is written by the C library to the underlying file
    descriptors, so neither the `quiet` flag nor `contextlib.redirect_stdout`
    can reach it -- the descriptors themselves have to be redirected.

    This hides informational output only. Genuine solver failures are raised as
    Python exceptions and still propagate.
    """
    sys.stdout.flush()
    sys.stderr.flush()
    saved = [os.dup(1), os.dup(2)]
    devnull = os.open(os.devnull, os.O_WRONLY)
    try:
        os.dup2(devnull, 1)
        os.dup2(devnull, 2)
        yield
    finally:
        sys.stdout.flush()
        sys.stderr.flush()
        os.dup2(saved[0], 1)
        os.dup2(saved[1], 2)
        os.close(devnull)
        for fd in saved:
            os.close(fd)


# --- The official DISORT flux-test problems ------------------------------
#
# Test 1  isotropic scattering, thin (tau = 0.03125) and thick (tau = 32)
# Test 2  Rayleigh scattering, moderate (tau = 0.2) and thick (tau = 5)
# Test 3  Henyey-Greenstein scattering, g = 0.75, conservative
#
# All cases use a single layer over a black Lambertian surface. The setups
# match the C drivers in tests/cdisort213/test_cdisort.c, and cases 1a-1f and
# 2a-2d are the same problems as tests/reference/test_problem_01_isotropic.py
# and tests/reference/test_problem_02_rayleigh.py.
#
# Beam illumination is normalised as fbeam = pi / mu0 in the official tests,
# so the downward flux at the top of the atmosphere is exactly pi.
BEAM_MU0 = 0.1  # tests 1a-1f
RAYLEIGH_MU0 = 0.080442  # tests 2a-2d
BEAM_FLUX = np.pi / BEAM_MU0

# fields: label, tau, single-scattering albedo, phase function, g, mu0,
#         beam flux, isotropic top illumination
CASES = [
    ("1a", 0.03125, 0.20, "isotropic", 0.0, BEAM_MU0, BEAM_FLUX, 0.0),
    ("1b", 0.03125, 1.00, "isotropic", 0.0, BEAM_MU0, BEAM_FLUX, 0.0),
    ("1c", 0.03125, 0.99, "isotropic", 0.0, BEAM_MU0, 0.0, 1.0),
    ("1d", 32.0, 0.20, "isotropic", 0.0, BEAM_MU0, BEAM_FLUX, 0.0),
    ("1e", 32.0, 1.00, "isotropic", 0.0, BEAM_MU0, BEAM_FLUX, 0.0),
    ("1f", 32.0, 0.99, "isotropic", 0.0, BEAM_MU0, 0.0, 1.0),
    ("2a", 0.2, 0.50, "rayleigh", 0.0, RAYLEIGH_MU0, np.pi, 0.0),
    ("2b", 0.2, 1.00, "rayleigh", 0.0, RAYLEIGH_MU0, np.pi, 0.0),
    ("2c", 5.0, 0.50, "rayleigh", 0.0, RAYLEIGH_MU0, np.pi, 0.0),
    ("2d", 5.0, 1.00, "rayleigh", 0.0, RAYLEIGH_MU0, np.pi, 0.0),
    ("3a", 1.0, 1.00, "henyey-greenstein", 0.75, 1.0, np.pi, 0.0),
    ("3b", 8.0, 1.00, "henyey-greenstein", 0.75, 1.0, np.pi, 0.0),
]

DESCRIPTIONS = {
    "1a": "thin, absorbing, beam",
    "1b": "thin, conservative, beam",
    "1c": "thin, isotropic top source",
    "1d": "thick, absorbing, beam",
    "1e": "thick, conservative, beam",
    "1f": "thick, isotropic top source",
    "2a": "Rayleigh, absorbing",
    "2b": "Rayleigh, conservative",
    "2c": "Rayleigh, thick, absorbing",
    "2d": "Rayleigh, thick, conservative",
    "3a": "forward-peaked, tau = 1",
    "3b": "forward-peaked, tau = 8",
}

# Published DISORT benchmark fluxes: (upward TOA, downward TOA, downward BOA).
# The upward flux at the bottom is zero in every case, because the surface is
# black. Everything else follows from these three.
REFERENCE = {
    "1a": (0.0799451, 3.14159, 2.37785),
    "1b": (0.422922, 3.14159, 2.71867),
    "1c": (0.0906556, 3.14159, 3.04897),
    "1d": (0.259686, 3.14159, 0.0),
    "1e": (3.0739, 3.14159, 0.0676954),
    "1f": (2.49618, 3.14159, 0.00460048),
    "2a": (0.0535063, 0.252716, 0.0652102),
    "2b": (0.125561, 0.252716, 0.127154),
    "2c": (0.062473, 0.252716, 0.000251683),
    "2d": (0.225915, 0.252716, 0.0268008),
    "3a": (0.247374, 3.14159, 2.89422),
    "3b": (1.59096, 3.14159, 1.55063),
}

# The stream count the published benchmark values were themselves computed at.
REFERENCE_STREAMS = 16
STREAM_SWEEP = (2, 4, 8, 16, 32)


def solve(nstr: int) -> dict:
    """Solve all twelve cases at `nstr` streams in one batched call.

    Returns a dict of (ncase,) arrays keyed by flux quantity.
    """
    ncol = len(CASES)

    op = DisortOptions().flags("onlyfl,lamber,quiet")
    op.ds().nlyr = 1
    op.ds().nstr = nstr
    op.ds().nmom = nstr
    op.ds().nphase = nstr
    op.nwave(1)
    op.ncol(ncol)
    ds = Disort(op)

    # Optical properties: one layer per case, laid out along the column axis.
    # Supply moments 1..nstr in addition to the implicit zeroth moment.
    # The moment at order nstr can affect delta-M scaling. Varying nstr here
    # changes both angular quadrature and the supplied moment expansion.
    prop = torch.zeros((1, ncol, 1, 2 + nstr))
    for i, (_, tau, ssalb, phase, gg, *_rest) in enumerate(CASES):
        prop[0, i, 0, 0] = tau
        prop[0, i, 0, 1] = ssalb
        prop[0, i, 0, 2:] = scattering_moments(nstr, phase, gg)

    zeros_wc = torch.zeros((1, ncol))
    with quiet_backend():
        # (ncol, nlvl, 2) with nlvl = 2 and the last axis [up, down]
        flx = ds.forward(
            prop,
            umu0=torch.tensor([c[5] for c in CASES]),
            phi0=torch.zeros(ncol),
            fbeam=torch.tensor([[c[6] for c in CASES]]),
            fisot=torch.tensor([[c[7] for c in CASES]]),
            albedo=zeros_wc,  # black surface
            fluor=zeros_wc,
        )[0].numpy()

    up_toa, down_toa = flx[:, 0, 0], flx[:, 0, 1]
    up_boa, down_boa = flx[:, 1, 0], flx[:, 1, 1]
    return {
        "up TOA": up_toa,
        "down TOA": down_toa,
        "down BOA": down_boa,
        "net TOA": up_toa - down_toa,
        "net BOA": up_boa - down_boa,
    }


def reference_values() -> dict:
    """The published benchmark fluxes, in the same layout as `solve`."""
    up_toa = np.array([REFERENCE[c[0]][0] for c in CASES])
    down_toa = np.array([REFERENCE[c[0]][1] for c in CASES])
    down_boa = np.array([REFERENCE[c[0]][2] for c in CASES])
    return {
        "up TOA": up_toa,
        "down TOA": down_toa,
        "down BOA": down_boa,
        "net TOA": up_toa - down_toa,
        "net BOA": -down_boa,  # the surface is black, so the upward flux is 0
    }


# Exclude imposed `down TOA` and black-surface upward flux from the scored
# quantities. Zero reference values have undefined relative error and are
# filtered below. Compare these statistics only with matching scoring rules.
SCORED = ("up TOA", "down BOA", "net TOA", "net BOA")


def relative_differences(result: dict, reference: dict) -> np.ndarray:
    """Percent relative differences over the scored, finite rows."""
    diffs = []
    for key in SCORED:
        ref, got = reference[key], result[key]
        nonzero = ref != 0.0
        diffs.append(100.0 * (got[nonzero] - ref[nonzero]) / ref[nonzero])
    return np.concatenate(diffs)


def summarise(diffs: np.ndarray) -> dict:
    magnitude = np.abs(diffs)
    return {
        "rows": magnitude.size,
        "median": float(np.median(magnitude)),
        "p75": float(np.percentile(magnitude, 75)),
        "p90": float(np.percentile(magnitude, 90)),
        "p95": float(np.percentile(magnitude, 95)),
        "max": float(magnitude.max()),
    }


def make_figure(reference: dict, sweep: dict, path: str) -> None:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    fig, axes = plt.subplots(1, 2, figsize=(11, 4.5))

    # (a) two-stream error per case
    ax = axes[0]
    labels = [c[0] for c in CASES]
    two = sweep[2]
    width = 0.27
    offsets = np.arange(len(CASES))
    for j, key in enumerate(("up TOA", "down BOA", "net TOA")):
        ref = reference[key]
        nonzero = ref != 0.0
        err = np.full(len(CASES), np.nan)
        err[nonzero] = (
            100.0 * (two[key][nonzero] - ref[nonzero]) / ref[nonzero]
        )
        ax.bar(offsets + (j - 1) * width, err, width, label=key)
    ax.set_xticks(offsets)
    ax.set_xticklabels(labels)
    ax.axhline(0.0, color="k", lw=0.6)
    ax.set_xlabel("DISORT flux-test case")
    ax.set_ylabel("two-stream error [%]")
    ax.set_title(
        "Two-stream error against published DISORT fluxes", fontsize=10
    )
    ax.legend(fontsize=8)

    # (b) convergence with stream count
    ax = axes[1]
    for stat, style in (("median", "-o"), ("p95", "--s"), ("max", ":^")):
        values = [
            summarise(relative_differences(sweep[n], reference))[stat]
            for n in STREAM_SWEEP
        ]
        ax.plot(STREAM_SWEEP, values, style, label=stat)
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xticks(STREAM_SWEEP)
    ax.set_xticklabels([str(n) for n in STREAM_SWEEP])
    ax.set_xlabel("number of streams")
    ax.set_ylabel("|relative difference| [%]")
    ax.set_title("Discrepancy from published values", fontsize=10)
    ax.grid(True, which="both", lw=0.3, alpha=0.5)
    ax.legend(fontsize=8)

    fig.tight_layout()
    fig.savefig(path, dpi=150)
    print(f"figure written to {path}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--plot",
        metavar="PATH",
        default=None,
        help="write a summary figure to PATH (needs matplotlib)",
    )
    args = parser.parse_args()

    reference = reference_values()

    print("DISORT stream-resolution study against published reference fluxes")
    print(
        f"  {len(CASES)} official DISORT flux-test cases, "
        f"solved in one batched call per stream count"
    )

    # --- 1. reproduce the published benchmark values ----------------------
    ref_run = solve(REFERENCE_STREAMS)

    print()
    print(
        f"1. pydisort at {REFERENCE_STREAMS} streams vs. the published "
        f"DISORT benchmark"
    )
    print(
        f"{'case':>5} {'description':>30} {'quantity':>10} "
        f"{'published':>13} {'pydisort':>13} {'diff %':>9}"
    )
    worst = 0.0
    for i, case in enumerate(CASES):
        label = case[0]
        description = DESCRIPTIONS[label]
        for key in ("up TOA", "down TOA", "down BOA"):
            ref, got = reference[key][i], ref_run[key][i]
            if ref == 0.0:
                diff = 0.0
                shown = "  (zero)"
            else:
                diff = 100.0 * (got - ref) / ref
                shown = f"{diff:>9.4f}"
                worst = max(worst, abs(diff))
            print(
                f"{label:>5} {description:>30} {key:>10} "
                f"{ref:>13.6g} {got:>13.6g} {shown:>9}"
            )
    print()
    print(f"largest deviation from the published values: {worst:.4f} %")

    # --- 2. the two-stream error ------------------------------------------
    sweep = {n: solve(n) for n in STREAM_SWEEP}
    two = sweep[2]

    print()
    print("2. Two-stream (nstr = 2) error against the reference")
    print(
        f"{'case':>5} {'description':>30} "
        + "".join(f"{k:>12}" for k in SCORED)
    )
    for i, case in enumerate(CASES):
        row = ""
        for key in SCORED:
            ref = reference[key][i]
            if ref == 0.0:
                row += f"{'--':>12}"
            else:
                row += f"{100.0 * (two[key][i] - ref) / ref:>12.2f}"
        print(f"{case[0]:>5} {DESCRIPTIONS[case[0]]:>30}" + row)

    stats = summarise(relative_differences(two, reference))
    print()
    print(
        f"over {stats['rows']} scored rows, |relative difference| in percent:"
    )
    print(
        f"  median {stats['median']:.2f}   75th {stats['p75']:.2f}   "
        f"90th {stats['p90']:.2f}   95th {stats['p95']:.2f}   "
        f"max {stats['max']:.2f}"
    )
    print()
    paragraph(
        """
        The largest relative discrepancies occur in two regimes. First,
        cases where the reference flux is nearly zero: 1f
        and 2c transmit almost nothing, so a tiny absolute error is a huge
        relative one. Second, the forward-peaked Henyey-Greenstein cases 3a and
        3b, where angular resolution matters.
        """
    )
    paragraph(
        """
        These percentiles score upward TOA, downward BOA and net boundary
        fluxes, excluding zero references and imposed boundary fluxes that
        are exact by construction. Comparisons with another solver require
        matching cases, output conventions and scoring rules; this selection
        alone does not establish a stricter validation.
        """
    )

    # --- 3. how many streams are enough? ----------------------------------
    print()
    print("3. Convergence with stream count")
    print(
        f"{'streams':>9} {'median':>10} {'75th':>10} {'90th':>10} "
        f"{'95th':>10} {'max':>10}"
    )
    sweep_stats = {}
    for nstr in STREAM_SWEEP:
        s = summarise(relative_differences(sweep[nstr], reference))
        sweep_stats[nstr] = s
        print(
            f"{nstr:>9} {s['median']:>10.4f} {s['p75']:>10.4f} "
            f"{s['p90']:>10.4f} {s['p95']:>10.4f} {s['max']:>10.4f}"
        )
    print()
    paragraph(
        """
        Four streams remove most of the two-stream discrepancy in these cases,
        and the sixteen-stream run closely matches the published values. This
        table measures sensitivity to angular resolution; it does not time
        the solves or establish a runtime/accuracy trade-off.
        """
    )
    paragraph(
        f"""
        Agreement with the tabulated values is best at {REFERENCE_STREAMS}
        streams and slightly worse at 32. These are finite-precision reference
        values, not an exact solution. This comparison alone cannot establish
        that 32 streams is more accurate or identify the cause of the residual
        differences; assess convergence for your own quantities and cases.
        """
    )

    if args.plot:
        make_figure(reference, sweep, args.plot)

    # --- validation -------------------------------------------------------
    print()

    # (1) External validation: at the reference stream count pydisort must
    #     reproduce the published DISORT benchmark values.
    print(f"published-benchmark check    : max deviation {worst:.4f} %")
    assert worst < 0.01, f"published benchmark not reproduced: {worst} %"

    # (2) Energy conservation: for a conservative layer over a black surface,
    #     everything that goes in must come back out or reach the surface.
    conservative = [i for i, c in enumerate(CASES) if c[2] == 1.0]
    residual = 0.0
    for i in conservative:
        incoming = ref_run["down TOA"][i]
        outgoing = ref_run["up TOA"][i] + ref_run["down BOA"][i]
        residual = max(residual, abs(incoming - outgoing) / incoming)
    print(
        f"conservative-scattering check: max relative residual {residual:.2e}"
    )
    assert residual < 1e-7, f"energy is not conserved: {residual}"

    # (3) For these cases the median discrepancy at 32 streams is below that
    #     at 2 streams. This does not assert monotonic convergence.
    medians = [sweep_stats[n]["median"] for n in STREAM_SWEEP]
    assert medians[0] > medians[-1], "increasing the stream count did not help"
    assert sweep_stats[REFERENCE_STREAMS]["max"] < 0.01
    print(
        "OK: published values reproduced, energy conserved, "
        "solution converges with stream count."
    )


if __name__ == "__main__":
    main()
