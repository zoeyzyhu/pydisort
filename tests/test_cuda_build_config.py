"""Check CUDA packaging decisions without requiring a GPU or native build."""

import ast
import os
from pathlib import Path
from types import SimpleNamespace

import pytest


@pytest.fixture
def cuda_build_enabled():
    setup_path = Path(__file__).resolve().parents[1] / "setup.py"
    if not setup_path.exists():
        pytest.skip("packaging check requires the source checkout")
    tree = ast.parse(setup_path.read_text())
    function = next(
        node
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and node.name == "cuda_build_enabled"
    )
    torch = SimpleNamespace(
        cuda=SimpleNamespace(is_available=lambda: False),
        version=SimpleNamespace(cuda="12.8"),
    )
    extension = SimpleNamespace(CUDA_HOME="/usr/local/cuda")
    namespace = {"os": os, "torch": torch, "cpp_extension": extension}
    exec(
        compile(
            ast.Module(body=[function], type_ignores=[]),
            str(setup_path),
            "exec",
        ),
        namespace,
    )
    return namespace["cuda_build_enabled"], torch, extension


def test_cuda_build_without_visible_gpu(cuda_build_enabled, monkeypatch):
    enabled, _, _ = cuda_build_enabled
    monkeypatch.setenv("PYDISORT_CUDA", "ON")
    assert enabled()


def test_explicit_cpu_build_on_gpu_host(cuda_build_enabled, monkeypatch):
    enabled, torch, _ = cuda_build_enabled
    torch.cuda.is_available = lambda: True
    monkeypatch.setenv("PYDISORT_CUDA", "OFF")
    assert not enabled()


@pytest.mark.parametrize("available", [False, True])
def test_default_preserves_gpu_detection(
    cuda_build_enabled, monkeypatch, available
):
    enabled, torch, _ = cuda_build_enabled
    torch.cuda.is_available = lambda: available
    monkeypatch.delenv("PYDISORT_CUDA", raising=False)
    assert enabled() == available


@pytest.mark.parametrize("missing", ["torch", "toolkit"])
def test_explicit_cuda_requires_toolchain(
    cuda_build_enabled, monkeypatch, missing
):
    enabled, torch, extension = cuda_build_enabled
    monkeypatch.setenv("PYDISORT_CUDA", "ON")
    if missing == "torch":
        torch.version.cuda = None
    else:
        extension.CUDA_HOME = None
    with pytest.raises(RuntimeError, match="CUDA-enabled PyTorch"):
        enabled()


def test_invalid_cuda_setting_fails(cuda_build_enabled, monkeypatch):
    enabled, _, _ = cuda_build_enabled
    monkeypatch.setenv("PYDISORT_CUDA", "invalid")
    with pytest.raises(ValueError, match="ON, OFF, or auto"):
        enabled()
