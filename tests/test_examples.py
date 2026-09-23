"""Run every script in `examples/` as a test.

The examples are self-checking: each one ends with assertions comparing its
output against a reference value, analytic limit, conservation law or consistency relation. Running them here
means a change in solver behaviour that breaks a documented example fails the
test suite rather than being discovered by a user.
"""

import pathlib
import runpy
import sys

import pytest

EXAMPLES = pathlib.Path(__file__).resolve().parent.parent / "examples"
SCRIPTS = sorted(EXAMPLES.glob("example_*.py"))


@pytest.mark.skipif(not SCRIPTS, reason="examples directory not available")
@pytest.mark.parametrize("script", SCRIPTS, ids=lambda p: p.stem)
def test_example_runs(script, monkeypatch, capsys):
    # The examples parse command-line arguments; make sure they see none of
    # pytest's own.
    monkeypatch.setattr(sys, "argv", [str(script)])
    runpy.run_path(str(script), run_name="__main__")

    # Every example finishes by printing an "OK:" line once its own checks
    # have passed.
    assert "OK:" in capsys.readouterr().out
