#!/usr/bin/env python3
"""Compare FP64 CPU and CUDA fluxes across representative DISORT cases."""

from __future__ import annotations

import argparse

import torch

from benchmark_cuda_fp64 import DTYPE, inputs, pydisort_solver


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--layers", type=int, default=40)
    parser.add_argument("--profiles", type=int, default=1)
    parser.add_argument(
        "--angles", type=float, nargs="+", default=(0.1, 0.5, 0.9)
    )
    parser.add_argument(
        "--optical-depths",
        type=float,
        nargs="+",
        default=(0.1, 1.0, 4.0, 20.0),
    )
    return parser.parse_args()


def solve(
    mode: str,
    scattering: bool,
    nstr: int,
    device: torch.device,
    nlayer: int,
    nprofile: int,
    umu0: float,
    optical_depth: float,
    albedo: float,
) -> torch.Tensor:
    prop, temperature, bc = inputs(
        mode, nprofile, nlayer, nstr, device, scattering
    )
    prop[..., 0] = optical_depth / nlayer
    if mode == "shortwave":
        bc["umu0"] = torch.full((nprofile,), umu0, dtype=DTYPE, device=device)
        bc["fbeam"] = torch.full(
            (1, nprofile), 100.0, dtype=DTYPE, device=device
        )
        bc["albedo"] = torch.full(
            (1, nprofile), albedo, dtype=DTYPE, device=device
        )
    solver = pydisort_solver(mode, nprofile, nlayer, nstr)
    output = (
        solver(prop, **bc)
        if temperature is None
        else solver(prop, temf=temperature, **bc)
    )
    if device.type == "cuda":
        torch.cuda.synchronize(device)
    return output.cpu()


def relative_rmse(
    value: torch.Tensor, reference: torch.Tensor
) -> float | None:
    scale = float(reference.square().mean().sqrt())
    error = float((value - reference).square().mean().sqrt())
    if scale:
        return error / scale
    return 0.0 if error == 0.0 else None


def format_percent(value: float | None) -> str:
    return "N/A" if value is None else f"{100 * value:.4g}"


def report(
    label: str,
    mode: str,
    scattering: bool,
    nstr: int,
    optical_depth: float,
    albedo: float,
    umu0: float,
    args: argparse.Namespace,
    cpu: torch.device,
    cuda: torch.device,
) -> None:
    reference = solve(
        mode,
        scattering,
        nstr,
        cpu,
        args.layers,
        args.profiles,
        umu0,
        optical_depth,
        albedo,
    )
    value = solve(
        mode,
        scattering,
        nstr,
        cuda,
        args.layers,
        args.profiles,
        umu0,
        optical_depth,
        albedo,
    )
    net_reference = reference[..., 0] - reference[..., 1]
    net_value = value[..., 0] - value[..., 1]
    max_abs = float((value - reference).abs().max())
    angle = f"{umu0:.2f}" if mode == "shortwave" else "-"
    print(
        f"{label:<24} {nstr:4d} {optical_depth:6.2g} {albedo:5.2f} {angle:>6} "
        f"{format_percent(relative_rmse(value[..., 0], reference[..., 0])):>11} "
        f"{format_percent(relative_rmse(value[..., 1], reference[..., 1])):>11} "
        f"{format_percent(relative_rmse(net_value, net_reference)):>11} {max_abs:12.4g}"
    )


def main() -> None:
    args = parse_args()
    if (
        args.layers < 1
        or args.profiles < 1
        or any(angle <= 0 for angle in args.angles)
        or any(depth <= 0 for depth in args.optical_depths)
    ):
        raise ValueError(
            "layers, profiles, angles, and optical depths must be positive"
        )
    if not torch.cuda.is_available():
        raise RuntimeError("CUDA is required")

    torch.set_num_threads(1)
    cpu = torch.device("cpu")
    cuda = torch.device("cuda")
    print("CPU-CUDA FP64 flux agreement")
    print("relative RMSE is %, max absolute error is in flux units")
    print(
        f"{'case':<24} {'nstr':>4} {'tau':>6} {'alb':>5} {'umu0':>6} {'up %':>11} "
        f"{'down %':>11} {'net %':>11} {'max abs':>12}"
    )
    for mode in ("shortwave", "longwave"):
        angles = args.angles if mode == "shortwave" else (0.0,)
        albedo = 0.1 if mode == "shortwave" else 0.0
        for scattering in (False, True):
            label = f"{mode} {'scattering' if scattering else 'clear-sky'}"
            for nstr in (4, 8):
                for optical_depth in args.optical_depths:
                    for umu0 in angles:
                        report(
                            label,
                            mode,
                            scattering,
                            nstr,
                            optical_depth,
                            albedo,
                            umu0,
                            args,
                            cpu,
                            cuda,
                        )
                if mode == "shortwave" and not scattering:
                    report(
                        "shortwave clear bright",
                        mode,
                        scattering,
                        nstr,
                        1.0,
                        0.8,
                        0.5,
                        args,
                        cpu,
                        cuda,
                    )


if __name__ == "__main__":
    main()
