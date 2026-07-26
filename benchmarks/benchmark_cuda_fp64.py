#!/usr/bin/env python3
"""Benchmark FP64 flux-only solvers with one CPU thread and CUDA.

Set ``EXOFMS_SOURCE_ROOT`` to an Exo-FMS column checkout to include its
Fortran Toon solvers; pyharp Toon is included when it is importable.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import shutil
import subprocess
import time
from dataclasses import asdict, dataclass
from pathlib import Path

import torch


DTYPE = torch.float64
MODES = ("shortwave", "longwave")
SCENARIOS = ("clear-sky", "scattering")
PDISORT_STREAMS = (4, 8)
WAVE_LOWER = [0.0]
WAVE_UPPER = [50000.0]


@dataclass
class Result:
    mode: str
    scattering: bool
    solver: str
    device: str
    profiles: int
    seconds: float


@dataclass
class Accuracy:
    case: str
    solver: str
    device: str
    flux_up: float
    flux_down: float
    net_flux: float


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--profiles", type=int, nargs="+", default=(1000, 10000, 100000)
    )
    parser.add_argument("--layers", type=int, default=40)
    parser.add_argument("--modes", choices=MODES, nargs="+", default=MODES)
    parser.add_argument("--scenarios", choices=SCENARIOS, nargs="+", default=SCENARIOS)
    parser.add_argument("--warmup", type=int, default=3)
    parser.add_argument("--repeats", type=int, default=10)
    parser.add_argument("--output", type=Path, default=Path("benchmark-results"))
    parser.add_argument(
        "--results",
        type=Path,
        nargs="+",
        help="plot one or more existing JSON result files instead of running solvers",
    )
    return parser.parse_args()


def time_call(call, device: torch.device, warmup: int, repeats: int) -> float:
    for _ in range(warmup):
        call()
    if device.type == "cuda":
        torch.cuda.synchronize(device)
        start = torch.cuda.Event(enable_timing=True)
        end = torch.cuda.Event(enable_timing=True)
        start.record()
        for _ in range(repeats):
            call()
        end.record()
        torch.cuda.synchronize(device)
        return start.elapsed_time(end) / 1_000.0 / repeats

    start = time.perf_counter()
    for _ in range(repeats):
        call()
    return (time.perf_counter() - start) / repeats


def scattering_properties(
    nprofile: int, nlayer: int, nstr: int, device: torch.device, scattering: bool
) -> torch.Tensor:
    options = {"device": device, "dtype": DTYPE}
    prop = torch.empty((1, nprofile, nlayer, 2 + nstr), **options)
    prop[..., 0] = 0.1
    prop[..., 1] = 0.5 if scattering else 0.0
    for moment in range(nstr):
        prop[..., 2 + moment] = 0.5 ** (moment + 1) if scattering else 0.0
    return prop


def thermal_profile(nprofile: int, nlayer: int, device: torch.device) -> torch.Tensor:
    options = {"device": device, "dtype": DTYPE}
    level = torch.arange(nlayer + 1, **options)
    top_to_bottom = torch.where(
        level <= 5,
        torch.ones_like(level),
        torch.where(
            level >= nlayer - 4,
            torch.full_like(level, 300.0),
            1.0 + 299.0 * (level - 5) / (nlayer - 10),
        ),
    )
    return top_to_bottom.flip(0).expand(nprofile, -1).contiguous()


def inputs(
    mode: str,
    nprofile: int,
    nlayer: int,
    nstr: int,
    device: torch.device,
    scattering: bool,
) -> tuple[torch.Tensor, torch.Tensor | None, dict[str, torch.Tensor]]:
    options = {"device": device, "dtype": DTYPE}
    prop = scattering_properties(nprofile, nlayer, nstr, device, scattering)
    if mode == "shortwave":
        return (
            prop,
            None,
            {
                "umu0": torch.full((nprofile,), 0.5, **options),
                "fbeam": torch.ones((1, nprofile), **options),
                "albedo": torch.full((1, nprofile), 0.1, **options),
            },
        )

    temperature = thermal_profile(nprofile, nlayer, device)
    return (
        prop,
        temperature,
        {
            "albedo": torch.zeros((1, nprofile), **options),
            "temis": torch.zeros((1, nprofile), **options),
            "btemp": temperature[:, 0],
            "ttemp": temperature[:, -1],
        },
    )


def pydisort_solver(mode: str, nprofile: int, nlayer: int, nstr: int):
    try:
        from pydisort.pydisort import Disort, DisortOptions
    except ModuleNotFoundError:
        from pydisort import Disort, DisortOptions

    options = DisortOptions()
    options.upward(True)
    options.flags("onlyfl,lamber,quiet" + (",planck" if mode == "longwave" else ""))
    options.nwave(1)
    options.ncol(nprofile)
    if mode == "longwave":
        options.wave_lower(WAVE_LOWER)
        options.wave_upper(WAVE_UPPER)
    options.ds().nlyr = nlayer
    options.ds().nstr = nstr
    options.ds().nmom = nstr
    options.ds().nphase = nstr
    return Disort(options)


def benchmark_pydisort(
    mode: str,
    scattering: bool,
    nprofile: int,
    nlayer: int,
    nstr: int,
    device: torch.device,
    warmup: int,
    repeats: int,
) -> Result:
    solver = pydisort_solver(mode, nprofile, nlayer, nstr)
    prop, temperature, bc = inputs(mode, nprofile, nlayer, nstr, device, scattering)

    def call():
        if temperature is None:
            return solver(prop, **bc)
        return solver(prop, temf=temperature, **bc)

    seconds = time_call(call, device, warmup, repeats)
    return Result(
        mode,
        scattering,
        f"pydisort DISORT {nstr}-stream",
        device.type.upper(),
        nprofile,
        seconds,
    )


def benchmark_pyharp(
    mode: str,
    scattering: bool,
    nprofile: int,
    nlayer: int,
    device: torch.device,
    warmup: int,
    repeats: int,
) -> Result:
    import pyharp

    toon_options = pyharp.ToonMcKay89Options()
    if mode == "longwave":
        toon_options.wave_lower(WAVE_LOWER)
        toon_options.wave_upper(WAVE_UPPER)
    solver = pyharp.ToonMcKay89(toon_options)
    prop, temperature, bc = inputs(mode, nprofile, nlayer, 1, device, scattering)

    def call():
        if temperature is None:
            return solver(prop[..., :3], **bc)
        return solver(prop[..., :3], temf=temperature, **bc)

    seconds = time_call(call, device, warmup, repeats)
    return Result(
        mode, scattering, "pyharp Toon", device.type.upper(), nprofile, seconds
    )


def benchmark_exofms(
    root: Path,
    profiles: list[int],
    nlayer: int,
    warmup: int,
    repeats: int,
    scattering: bool,
) -> list[Result]:
    runner = Path(__file__).with_name("run_exofms_toon_benchmark.sh")
    env = os.environ | {
        "OMP_NUM_THREADS": "1",
        "EXOFMS_LAYERS": str(nlayer),
        "EXOFMS_WARMUP": str(warmup),
        "EXOFMS_REPEATS": str(repeats),
        "EXOFMS_SSA": "0.5" if scattering else "0.0",
        "EXOFMS_ASYMMETRY": "0.5" if scattering else "0.0",
    }
    completed = subprocess.run(
        [str(runner), str(root), *(str(value) for value in profiles)],
        check=True,
        capture_output=True,
        text=True,
        env=env,
    )
    result = []
    current_profiles = None
    fields_to_mode = {
        "exofms_sw_toon_seconds": "shortwave",
        "exofms_lw_toon_5node_seconds": "longwave",
    }
    for line in completed.stdout.splitlines():
        fields = dict(item.split("=", 1) for item in line.split(",") if "=" in item)
        if "nprofile" in fields:
            current_profiles = int(fields["nprofile"])
        elif current_profiles:
            for field, mode in fields_to_mode.items():
                if field in fields:
                    result.append(
                        Result(
                            mode,
                            scattering,
                            "Fortran Toon",
                            "CPU",
                            current_profiles,
                            float(fields[field]),
                        )
                    )
    if len(result) != 2 * len(profiles):
        raise RuntimeError(
            f"could not parse Exo-FMS benchmark output:\n{completed.stdout}"
        )
    return result


def exofms_accuracy(
    root: Path, nlayer: int, scattering: bool
) -> tuple[torch.Tensor, dict[str, torch.Tensor]]:
    runner = Path(__file__).with_name("run_exofms_toon_benchmark.sh")
    env = os.environ | {
        "OMP_NUM_THREADS": "1",
        "EXOFMS_LAYERS": str(nlayer),
        "EXOFMS_WARMUP": "0",
        "EXOFMS_REPEATS": "1",
        "EXOFMS_SSA": "0.5" if scattering else "0.0",
        "EXOFMS_ASYMMETRY": "0.5" if scattering else "0.0",
        "EXOFMS_PRINT_ACCURACY": "1",
    }
    completed = subprocess.run(
        [str(runner), str(root), "1"],
        check=True,
        capture_output=True,
        text=True,
        env=env,
    )
    rows = []
    for line in completed.stdout.splitlines():
        fields = dict(item.split("=", 1) for item in line.split(",") if "=" in item)
        if "accuracy_level" in fields:
            rows.append(fields)
    if len(rows) != nlayer + 1:
        raise RuntimeError(
            f"could not parse Exo-FMS accuracy output:\n{completed.stdout}"
        )
    fields = ("temperature", "sw_up", "sw_down", "lw_up", "lw_down")
    output = {
        name: torch.tensor([float(row[name]) for row in rows], dtype=DTYPE)
        for name in fields
    }
    temperature = output.pop("temperature").flip(0).view(1, -1)
    return temperature, {
        "shortwave": torch.stack((output["sw_up"], output["sw_down"]), dim=-1).flip(0),
        "longwave": torch.stack((output["lw_up"], output["lw_down"]), dim=-1).flip(0),
    }


def flux_pydisort(
    mode: str,
    scattering: bool,
    nlayer: int,
    nstr: int,
    device: torch.device,
    temperature: torch.Tensor | None,
) -> torch.Tensor:
    solver = pydisort_solver(mode, 1, nlayer, nstr)
    prop, _, bc = inputs(mode, 1, nlayer, nstr, device, scattering)
    if temperature is None:
        flux = solver(prop, **bc)
    else:
        temperature = temperature.to(device)
        bc["btemp"] = temperature[:, 0]
        bc["ttemp"] = temperature[:, -1]
        flux = solver(prop, temf=temperature, **bc)
    if device.type == "cuda":
        torch.cuda.synchronize(device)
    return flux.cpu().squeeze((0, 1))


def flux_pyharp(
    mode: str,
    scattering: bool,
    nlayer: int,
    device: torch.device,
    temperature: torch.Tensor | None,
) -> torch.Tensor:
    import pyharp

    options = pyharp.ToonMcKay89Options()
    if mode == "longwave":
        options.wave_lower(WAVE_LOWER)
        options.wave_upper(WAVE_UPPER)
    solver = pyharp.ToonMcKay89(options)
    prop, _, bc = inputs(mode, 1, nlayer, 1, device, scattering)
    flux = (
        solver(prop[..., :3], **bc)
        if temperature is None
        else solver(prop[..., :3], temf=temperature.to(device), **bc)
    )
    if device.type == "cuda":
        torch.cuda.synchronize(device)
    return flux.cpu().squeeze((0, 1))


def relative_rmse(value: torch.Tensor, reference: torch.Tensor) -> float:
    scale = max(float(torch.mean(reference.square()).sqrt()), 1.0)
    return float(torch.mean((value - reference).square()).sqrt() / scale)


def score_accuracy(
    value: torch.Tensor, reference: torch.Tensor
) -> tuple[float, float, float]:
    return (
        relative_rmse(value[:, 0], reference[:, 0]),
        relative_rmse(value[:, 1], reference[:, 1]),
        relative_rmse(value[:, 0] - value[:, 1], reference[:, 0] - reference[:, 1]),
    )


def benchmark_accuracy(
    root: Path,
    nlayer: int,
    modes: list[str],
    scatterings: list[bool],
    pyharp_available: bool,
) -> list[Accuracy]:
    rows = []
    devices = [torch.device("cpu")]
    if torch.cuda.is_available():
        devices.append(torch.device("cuda"))
    for scattering in scatterings:
        label = "scattering" if scattering else "clear-sky"
        temperature, fortran_fluxes = exofms_accuracy(root, nlayer, scattering)
        for mode in modes:
            thermal = temperature if mode == "longwave" else None
            reference = flux_pydisort(
                mode, scattering, nlayer, 16, torch.device("cpu"), thermal
            )
            candidates = [("Fortran Toon", "CPU", fortran_fluxes[mode])]
            if pyharp_available:
                for device in devices:
                    candidates.append(
                        (
                            "pyharp Toon",
                            device.type.upper(),
                            flux_pyharp(mode, scattering, nlayer, device, thermal),
                        )
                    )
            for nstr in PDISORT_STREAMS:
                for device in devices:
                    candidates.append(
                        (
                            f"pydisort DISORT {nstr}-stream",
                            device.type.upper(),
                            flux_pydisort(
                                mode, scattering, nlayer, nstr, device, thermal
                            ),
                        )
                    )
            for solver, device, flux in candidates:
                flux_up, flux_down, net_flux = score_accuracy(flux, reference)
                rows.append(
                    Accuracy(
                        f"{mode} {label}", solver, device, flux_up, flux_down, net_flux
                    )
                )
    return rows


def print_accuracy(rows: list[Accuracy]) -> None:
    print("\nAccuracy relative to DISORT 16-stream CPU (relative RMSE, %)")
    print(
        f"{'case':<24} {'solver':<27} {'device':<6} {'up':>10} {'down':>10} {'net':>10}"
    )
    for row in rows:
        print(
            f"{row.case:<24} {row.solver:<27} {row.device:<6} "
            f"{100 * row.flux_up:10.4g} {100 * row.flux_down:10.4g} {100 * row.net_flux:10.4g}"
        )


def plot(results: list[Result], output: Path) -> None:
    import matplotlib.pyplot as plt
    from matplotlib.patches import Patch

    profiles = sorted({result.profiles for result in results})
    solver_order = [
        "Fortran Toon",
        "pyharp Toon",
        "pydisort DISORT 4-stream",
        "pydisort DISORT 8-stream",
    ]
    solvers = [
        solver
        for solver in solver_order
        if any(result.solver == solver for result in results)
    ]
    colors = {
        "Fortran Toon": "#6f7782",
        "pyharp Toon": "#3366a5",
        "pydisort DISORT 4-stream": "#d9822b",
        "pydisort DISORT 8-stream": "#16877c",
    }
    cases = (
        ("shortwave", False, "Shortwave · clear sky"),
        ("shortwave", True, "Shortwave · scattering"),
        ("longwave", False, "Longwave · clear sky"),
        ("longwave", True, "Longwave · scattering"),
    )
    cases = [
        case
        for case in cases
        if any(
            result.mode == case[0] and result.scattering == case[1]
            for result in results
        )
    ]
    all_values = [1e6 * result.seconds / result.profiles for result in results]
    ymin, ymax = min(all_values) * 0.55, max(all_values) * 3.2

    def value_label(value: float) -> str:
        if value >= 100:
            return f"{value:.0f}"
        if value >= 1:
            return f"{value:.2f}".rstrip("0").rstrip(".")
        return f"{value:.3f}".rstrip("0").rstrip(".")

    figure, axes = plt.subplots(
        len(cases),
        len(profiles),
        figsize=(4.2 * len(profiles), 3.1 * len(cases)),
        sharey=True,
        squeeze=False,
    )
    for row, (mode, scattering, label) in enumerate(cases):
        selected = [
            result
            for result in results
            if result.mode == mode and result.scattering == scattering
        ]
        for column, (axis, nprofile) in enumerate(zip(axes[row], profiles)):
            positions = []
            values = []
            styles = []
            for index, solver in enumerate(solvers):
                devices = [
                    device
                    for device in ("CPU", "CUDA")
                    if any(
                        result.profiles == nprofile
                        and result.solver == solver
                        and result.device == device
                        for result in selected
                    )
                ]
                offsets = (
                    {"CPU": -0.19, "CUDA": 0.19}
                    if len(devices) == 2
                    else {devices[0]: 0.0}
                )
                for device in devices:
                    value = next(
                        1e6 * result.seconds / result.profiles
                        for result in selected
                        if result.profiles == nprofile
                        and result.solver == solver
                        and result.device == device
                    )
                    positions.append(index + offsets[device])
                    values.append(value)
                    styles.append(
                        (
                            colors[solver],
                            0.55 if device == "CPU" else 1.0,
                            "" if device == "CPU" else "//",
                        )
                    )
            bars = axis.bar(
                positions,
                values,
                width=0.34,
                color=[style[0] for style in styles],
                edgecolor="#333333",
                linewidth=0.5,
            )
            for bar, style in zip(bars, styles):
                bar.set_alpha(style[1])
                bar.set_hatch(style[2])
            for bar, value in zip(bars, values):
                axis.text(
                    bar.get_x() + bar.get_width() / 2,
                    value * 1.12,
                    value_label(value),
                    ha="center",
                    va="bottom",
                    fontsize=7,
                )
            if row == 0:
                axis.set_title(f"{nprofile:,} profiles")
            axis.set_xticks(
                range(len(solvers)),
                [
                    solver.replace("pydisort ", "").replace(" ", "\n", 1)
                    for solver in solvers
                ],
                fontsize=7,
            )
            axis.grid(axis="y", color="#d0d0d0", linewidth=0.6)
            axis.set_axisbelow(True)
            if column == 0:
                axis.set_ylabel(f"{label}\ntime / profile (µs)")
            axis.set_yscale("log")
            axis.set_ylim(ymin, ymax)
    figure.suptitle(
        "FP64 flux-only benchmark · 40 layers · one CPU thread",
        y=0.995,
    )
    figure.legend(
        handles=[
            *[
                Patch(
                    facecolor=colors[solver],
                    edgecolor="#333333",
                    label=solver.replace("pydisort ", ""),
                )
                for solver in solvers
            ],
            Patch(facecolor="#777777", alpha=0.55, edgecolor="#333333", label="CPU"),
            Patch(facecolor="#777777", edgecolor="#333333", hatch="//", label="CUDA"),
        ],
        loc="lower center",
        ncol=6,
        frameon=False,
        bbox_to_anchor=(0.5, -0.01),
    )
    figure.tight_layout(rect=(0.02, 0.04, 1.0, 0.97))
    figure.savefig(
        output / "fp64_solver_benchmark.png",
        dpi=240,
        bbox_inches="tight",
        pad_inches=0.16,
    )
    plt.close(figure)


def main() -> None:
    args = parse_args()
    if min(*args.profiles, args.repeats) < 1 or args.layers < 11 or args.warmup < 0:
        raise ValueError(
            "profiles and repeats must be positive; layers must be at least 11"
        )

    if args.results:
        results = []
        for path in args.results:
            with path.open() as stream:
                results.extend(
                    Result(scattering=row.pop("scattering", True), **row)
                    for row in json.load(stream)
                )
        args.output.mkdir(parents=True, exist_ok=True)
        try:
            plot(results, args.output)
        except ImportError:
            print("matplotlib not found; skipping plot")
        return

    torch.set_num_threads(1)
    devices = [torch.device("cpu")]
    if torch.cuda.is_available():
        devices.append(torch.device("cuda"))
    results = []
    scatterings = [scenario == "scattering" for scenario in args.scenarios]
    for scattering in scatterings:
        for mode in args.modes:
            for nprofile in args.profiles:
                for device in devices:
                    for nstr in PDISORT_STREAMS:
                        results.append(
                            benchmark_pydisort(
                                mode,
                                scattering,
                                nprofile,
                                args.layers,
                                nstr,
                                device,
                                args.warmup,
                                args.repeats,
                            )
                        )

    pyharp_available = False
    try:
        import pyharp  # noqa: F401
    except ImportError:
        print("pyharp not found; skipping pyharp Toon")
    else:
        pyharp_available = True
        for scattering in scatterings:
            for mode in args.modes:
                for nprofile in args.profiles:
                    for device in devices:
                        results.append(
                            benchmark_pyharp(
                                mode,
                                scattering,
                                nprofile,
                                args.layers,
                                device,
                                args.warmup,
                                args.repeats,
                            )
                        )

    exofms_root = os.environ.get("EXOFMS_SOURCE_ROOT")
    exofms_files = ("src/WENO4_mod.f90", "src/sw_Toon_mod.f90", "src/lw_Toon_mod.f90")
    exofms_available = (
        exofms_root
        and shutil.which(os.environ.get("FC", "gfortran"))
        and all((Path(exofms_root) / name).is_file() for name in exofms_files)
    )
    if exofms_available:
        for scattering in scatterings:
            results.extend(
                benchmark_exofms(
                    Path(exofms_root),
                    args.profiles,
                    args.layers,
                    args.warmup,
                    args.repeats,
                    scattering,
                )
            )
    else:
        print(
            "Exo-FMS Toon sources or Fortran compiler not found; skipping Fortran Toon"
        )

    args.output.mkdir(parents=True, exist_ok=True)
    with (args.output / "fp64_solver_benchmark.json").open("w") as stream:
        json.dump([asdict(result) for result in results], stream, indent=2)
    with (args.output / "fp64_solver_benchmark.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=Result.__annotations__)
        writer.writeheader()
        writer.writerows(asdict(result) for result in results)
    if exofms_available:
        print_accuracy(
            benchmark_accuracy(
                Path(exofms_root),
                args.layers,
                args.modes,
                scatterings,
                pyharp_available,
            )
        )
    try:
        plot(results, args.output)
    except ImportError:
        print("matplotlib not found; skipping plot")
    for result in results:
        print(
            f"{result.mode:9s} {result.solver:24s} {result.device:4s} {result.profiles:7d} {1e6 * result.seconds / result.profiles:10.3f} µs/profile"
        )


if __name__ == "__main__":
    main()
