"""Repository-only developer check for source-build installation guidance.

Run this file from the source checkout; CTest's build-tree copy skips it.
"""

import ast
import builtins
from pathlib import Path

import pytest


def test_missing_torch_recommends_a_wheel(monkeypatch):
    setup_path = Path(__file__).resolve().parents[1] / "setup.py"
    if not setup_path.exists():
        pytest.skip(
            "developer check requires the source checkout; run "
            "python -m pytest tests/test_setup_guidance.py there"
        )

    # Exercise the import guard without invoking setuptools or a native build.
    tree = ast.parse(setup_path.read_text())
    guard = next(node for node in tree.body if isinstance(node, ast.Try))
    module = ast.Module(body=[guard], type_ignores=[])
    original_import = builtins.__import__

    def without_torch(name, *args, **kwargs):
        if name == "torch":
            raise ModuleNotFoundError("No module named 'torch'", name="torch")
        return original_import(name, *args, **kwargs)

    monkeypatch.setattr(builtins, "__import__", without_torch)
    with pytest.raises(ModuleNotFoundError) as error:
        exec(compile(module, str(setup_path), "exec"), {})

    message = str(error.value)
    assert "python -m pip install --only-binary=pydisort pydisort" in message
    assert "installation.html" in message
    assert "cmake -" not in message
    assert "pip install --no-build-isolation ." not in message
