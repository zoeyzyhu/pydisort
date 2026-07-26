#!/usr/bin/env python3
"""Validate CUDA routing between fast-flux and general DISORT paths."""

from __future__ import annotations

import argparse
import json

import torch

try:
    from pydisort.pydisort import Disort, DisortOptions
except ModuleNotFoundError:
    from pydisort import Disort, DisortOptions


DTYPE = torch.float64
NLYR = 20
CONSERVATIVE_THRESHOLD = 1.0e-8
FAST_RELATIVE_TOLERANCE = 2.0e-6
GENERAL_RELATIVE_TOLERANCE = 1.0e-8


def make_solver(nwave: int, ncol: int, nstr: int) -> Disort:
    options = DisortOptions()
    options.upward(True)
    options.flags("onlyfl,lamber,quiet")
    options.nwave(nwave)
    options.ncol(ncol)
    options.ds().nlyr = NLYR
    options.ds().nstr = nstr
    options.ds().nmom = nstr
    options.ds().nphase = nstr
    return Disort(options)


def make_inputs(
    nwave: int, ncol: int, ssalb: torch.Tensor, nstr: int
) -> tuple[torch.Tensor, dict[str, torch.Tensor]]:
    device = ssalb.device
    prop = torch.empty((nwave, ncol, NLYR, 2 + nstr), device=device, dtype=DTYPE)
    prop[..., 0] = 0.5
    prop[..., 1] = ssalb
    for moment in range(nstr):
        prop[..., 2 + moment] = 0.85 ** (moment + 1)

    wave = torch.arange(nwave, device=device, dtype=DTYPE).view(nwave, 1)
    col = torch.arange(ncol, device=device, dtype=DTYPE).view(1, ncol)
    return prop, {
        "umu0": 0.5 + 0.05 * torch.arange(ncol, device=device, dtype=DTYPE),
        "fbeam": 100.0 + 7.0 * wave + col,
        "albedo": torch.zeros((nwave, ncol), device=device, dtype=DTYPE),
    }


def compare_case(ssalb: torch.Tensor, nstr: int) -> tuple[torch.Tensor, float, float]:
    nwave, ncol, _ = ssalb.shape
    solver = make_solver(nwave, ncol, nstr)
    prop_cuda, bc_cuda = make_inputs(nwave, ncol, ssalb.cuda(), nstr)
    gpu = solver(prop_cuda, **bc_cuda)
    torch.cuda.synchronize()
    cpu = solver(
        prop_cuda.cpu(), **{name: value.cpu() for name, value in bc_cuda.items()}
    )
    difference = (gpu.cpu() - cpu).abs()
    return (
        difference.amax(dim=(-1, -2)),
        float(difference.max()),
        float(cpu.abs().max()),
    )


def boundary_scan(nstr: int) -> list[dict[str, float]]:
    results = []
    for deficit in (1e-8, 1e-9, 1e-10, 1e-12, 0.0):
        ssalb = torch.full((1, 1, NLYR), 1.0 - deficit, dtype=DTYPE)
        _, max_abs, scale = compare_case(ssalb, nstr)
        results.append(
            {
                "one_minus_ssalb": deficit,
                "max_abs": max_abs,
                "relative": max_abs / max(scale, 1.0),
            }
        )
    return results


def conservative_batch(nstr: int) -> dict[str, float]:
    ssalb = torch.ones((2, 4, NLYR), dtype=DTYPE)
    _, max_abs, scale = compare_case(ssalb, nstr)
    return {
        "max_abs": max_abs,
        "relative": max_abs / max(scale, 1.0),
    }


def mixed_batch(nstr: int) -> dict[str, object]:
    ssalb = (
        torch.tensor(
            [[0.5, 1.0 - 1e-9, 0.9, 1.0], [0.9999, 0.99, 1.0 - 1e-12, 0.7]],
            dtype=DTYPE,
        )
        .unsqueeze(-1)
        .expand(2, 4, NLYR)
        .contiguous()
    )
    per_column, max_abs, scale = compare_case(ssalb, nstr)
    return {
        "per_wave_column_max_abs": per_column.tolist(),
        "max_abs": max_abs,
        "relative": max_abs / max(scale, 1.0),
    }


def regular_controls(nstr: int) -> list[dict[str, float]]:
    results = []
    for value in (0.5, 0.9, 0.99, 0.9999):
        ssalb = torch.full((1, 1, NLYR), value, dtype=DTYPE)
        _, max_abs, scale = compare_case(ssalb, nstr)
        results.append(
            {"ssalb": value, "max_abs": max_abs, "relative": max_abs / max(scale, 1.0)}
        )
    return results


def validate(results: dict[str, object]) -> None:
    for result in results["regular_controls"]:
        if result["relative"] > FAST_RELATIVE_TOLERANCE:
            raise AssertionError(f"regular control exceeded tolerance: {result}")

    for result in results["boundary_scan"]:
        tolerance = (
            FAST_RELATIVE_TOLERANCE
            if result["one_minus_ssalb"] >= CONSERVATIVE_THRESHOLD
            else GENERAL_RELATIVE_TOLERANCE
        )
        if result["relative"] > tolerance:
            raise AssertionError(f"boundary case exceeded tolerance: {result}")

    for name in ("conservative_batch", "mixed_batch"):
        result = results[name]
        if result["relative"] > GENERAL_RELATIVE_TOLERANCE:
            raise AssertionError(f"{name} exceeded tolerance: {result}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--nstr", type=int, default=8, choices=(4, 8))
    parser.add_argument("--no-assert", action="store_true")
    args = parser.parse_args()
    results = {
        "nstr": args.nstr,
        "boundary_scan": boundary_scan(args.nstr),
        "conservative_batch": conservative_batch(args.nstr),
        "mixed_batch": mixed_batch(args.nstr),
        "regular_controls": regular_controls(args.nstr),
    }
    if not args.no_assert:
        validate(results)
    print(json.dumps(results, indent=2, sort_keys=True))
