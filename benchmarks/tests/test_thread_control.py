"""Check benchmark thread limits without changing the pytest process's pools."""

import os
from pathlib import Path
import subprocess
import sys
import textwrap

import pytest


BENCHMARKS = Path(__file__).resolve().parents[1]
THREAD_VARIABLES = (
    "OPENBLAS_NUM_THREADS",
    "GOTO_NUM_THREADS",
    "BLIS_NUM_THREADS",
    "MKL_NUM_THREADS",
    "VECLIB_MAXIMUM_THREADS",
    "NUMEXPR_NUM_THREADS",
    "OMP_NUM_THREADS",
)


def run_probe(source):
    env = dict(os.environ, **{name: "4" for name in THREAD_VARIABLES})
    result = subprocess.run(
        [sys.executable, "-c", textwrap.dedent(source)],
        cwd=BENCHMARKS,
        env=env,
        text=True,
        capture_output=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr


def test_limits_override_environment_and_loaded_pools():
    pytest.importorskip("threadpoolctl")
    pytest.importorskip("PythonicDISORT")
    run_probe(
        """
        import os
        import numpy as np
        import scipy.linalg
        import torch
        from threadpoolctl import threadpool_info

        # Import the numerical runtimes before the benchmark: environment
        # settings alone can no longer control their initialized pools.
        torch.set_num_threads(4)
        import compare_pythonicdisort as bench

        for name in (
            "OPENBLAS_NUM_THREADS", "GOTO_NUM_THREADS", "BLIS_NUM_THREADS",
            "MKL_NUM_THREADS", "VECLIB_MAXIMUM_THREADS",
            "NUMEXPR_NUM_THREADS", "OMP_NUM_THREADS",
        ):
            assert os.environ[name] == "1", name

        def pool_counts():
            return {p['filepath']: p['num_threads'] for p in threadpool_info()}

        checks = []
        def check_single_thread():
            pools = pool_counts()
            assert pools and all(n == 1 for n in pools.values()), pools
            checks.append(pools)

        # Verify that the limit covers both the solve and deferred output
        # evaluation, and restores pre-existing pool sizes afterward.
        def up(tau):
            check_single_thread()
            return np.zeros_like(tau)

        def down(tau):
            check_single_thread()
            return np.zeros_like(tau), np.zeros_like(tau)

        def solve(radiance):
            check_single_thread()
            return None, up, down

        bench.pythonicdisort_call = solve
        before = pool_counts()
        bench.pythonicdisort_flux(False)
        assert len(checks) == 3
        assert pool_counts() == before

        # Reapply the limits after pydisort switches to several threads.
        bench.pythonicdisort_solve_and_evaluate = (
            lambda radiance: check_single_thread()
        )
        bench.time_pydisort(1, 2, 1, False)
        assert torch.get_num_threads() == 2
        before = pool_counts()
        for _ in range(2):
            bench.time_pythonicdisort(2, 1, False)
            assert pool_counts() == before
            assert torch.get_num_threads() == 2
        assert len(checks) == 9  # verification + two (warmup + 2 solves)
        """
    )


@pytest.mark.parametrize("mode", ["import", "cli"])
def test_missing_threadpoolctl_has_install_guidance(mode):
    run_probe(
        f"""
        import importlib.abc
        import runpy
        import sys

        class MissingThreadpoolctl(importlib.abc.MetaPathFinder):
            def find_spec(self, fullname, path, target=None):
                if fullname == 'threadpoolctl':
                    raise ModuleNotFoundError(fullname, name=fullname)

        sys.meta_path.insert(0, MissingThreadpoolctl())
        try:
            if {mode!r} == 'cli':
                runpy.run_path('compare_pythonicdisort.py', run_name='__main__')
            else:
                import compare_pythonicdisort
        except (SystemExit, ModuleNotFoundError) as exc:
            assert 'python -m pip install threadpoolctl' in str(exc)
            if {mode!r} == 'cli':
                assert isinstance(exc, SystemExit)
            else:
                assert isinstance(exc, ModuleNotFoundError)
                assert exc.name == 'threadpoolctl'
        else:
            raise AssertionError('benchmark must require threadpoolctl')
        """
    )


@pytest.mark.parametrize("missing", ["threadpoolctl", "PythonicDISORT"])
def test_plotting_without_optional_dependency(missing):
    run_probe(
        f"""
        import importlib.abc
        import sys
        from pathlib import Path

        class MissingOptionalDependency(importlib.abc.MetaPathFinder):
            def find_spec(self, fullname, path, target=None):
                if fullname == {missing!r}:
                    raise ModuleNotFoundError(fullname, name=fullname)

        sys.meta_path.insert(0, MissingOptionalDependency())
        import plot_scaling as plot
        assert not plot.HAVE_PYTHONICDISORT

        # --help must still work without the optional series.
        sys.argv = ['plot_scaling.py', '--help']
        try:
            plot.main()
        except SystemExit as exc:
            assert exc.code == 0
        else:
            raise AssertionError('--help should exit successfully')

        # Exercise the three-series fallback without compiling or timing.
        plot.cdi.build_cdisort = lambda path: (Path('unused'), 'c++', 'test')
        plot.cdi.verify = lambda *a, **k: (True, 0.0)
        plot.cdi.time_cdisort = lambda *a, **k: 1.0
        plot.cdi.time_pydisort = lambda *a, **k: 1.0
        results, _, _ = plot.measure([1], 2, 1, False, 1, Path('unused'))
        assert len(results) == 3
        assert plot.PYTHONICDISORT not in results
        """
    )


def test_plotting_does_not_hide_unrelated_import_errors():
    run_probe(
        """
        import importlib.abc
        import sys

        class BrokenBenchmarkImport(importlib.abc.MetaPathFinder):
            def find_spec(self, fullname, path, target=None):
                if fullname == 'compare_pythonicdisort':
                    raise ModuleNotFoundError('unrelated', name='unrelated')

        sys.meta_path.insert(0, BrokenBenchmarkImport())
        try:
            import plot_scaling
        except ModuleNotFoundError as exc:
            assert exc.name == 'unrelated'
        else:
            raise AssertionError('unrelated import failure was hidden')
        """
    )
