# pydisort benchmarks

Performance measurements for pydisort.

| Script | What it measures |
| --- | --- |
| [`compare_cdisort.py`](compare_cdisort.py) | pydisort vs. cdisort: what the Python wrapper costs, and what batching buys. Builds its own C baseline — no cmake, no libtorch. |
| [`compare_pythonicdisort.py`](compare_pythonicdisort.py) | pydisort vs. PythonicDISORT: the interpreted/compiled gap. |
| [`testproblem09.py`](testproblem09.py) | The shared problem definition. Not a benchmark — imported by both scripts above, so the configuration is written down exactly once. |
| [`bench_cdisort.cpp`](bench_cdisort.cpp) | The C baseline, compiled on demand by `compare_cdisort.py`. Not run directly. |

## What is measured

Spectral resolution is the dimension that makes radiative transfer expensive,
and it is the dimension pydisort parallelizes over. A single-column,
single-wavenumber solve is cheap; a realistic calculation repeats it over
hundreds or thousands of spectral points.

Both comparisons solve DISORT Test Problem 9 ("General Emitting/Absorbing/
Scattering") with **32 streams and 100 layers**, a workload typical of
atmospheric radiation calculations, and repeat it for a growing number of
wavenumbers.

They answer two different questions, which is why they are separate scripts:

* **`compare_cdisort.py`** — pydisort wraps cdisort, so this measures what the
  wrapper costs. The single-core ratio should be 1.0; the rest is what
  batching over wavenumbers buys. This is the durable result: it does not
  depend on a third-party package's release.
* **`compare_pythonicdisort.py`** — [PythonicDISORT](https://doi.org/10.21105/joss.06442)
  (Ho 2024) is a pure-Python reimplementation, included to show the
  interpreted/compiled gap.

Both import their problem definition from `testproblem09.py`, so the thing
being timed is defined in one place rather than copied into each script.

## Against cdisort

```bash
python benchmarks/compare_cdisort.py --verify-only   # seconds
python benchmarks/compare_cdisort.py                 # default sweep
python benchmarks/compare_cdisort.py --nwave 1,10,100,1000,10000 \
    --threads 10 --repeat 3
```

It needs no prepared build. cdisort is header-only (`cdisort213/` is a CMake
`INTERFACE` target), so the script compiles its own baseline from
[`bench_cdisort.cpp`](bench_cdisort.cpp) with the repository's Release flags
and nothing but a C++17 compiler and libm. The binary is cached in
`benchmarks/_bench_build/` and rebuilt when the source changes.

Three properties of that baseline each move the number:

* **The solve loop is timed inside the C program.** Running a driver as a
  subprocess and subtracting an estimated process startup leaves a residual of
  about 0.7% on this machine, and it errs in the direction that flatters
  pydisort. Timing with `std::chrono` inside the loop removes the correction
  rather than modelling it.
* **The baseline is cross-validated against pydisort.** `bench_cdisort.cpp`
  has a `--verify` mode, so the two are compared directly before any timing.
  They agree to ~1e-23: pydisort calls the same solver, so anything worse
  would mean the inputs had drifted apart.
* **Allocation policy is a switch, not an assumption.**
  `tests/cdisort213/test_cdisort_09.c` calls `c_disort_state_alloc`/`_free` on
  every iteration; pydisort allocates once per batch. `--alloc both` measures
  the difference so it cannot hide inside the ratio. On this machine the two
  are within **0.3%** of each other, i.e. indistinguishable from run-to-run
  noise.

At `--nstr 8 --nlyr 6 --ssalb 0.05` the driver reproduces the published Test
Problem 9a reference values to within their six-figure precision, which is
what establishes that the baseline solves the intended problem.

| Option | Meaning |
| --- | --- |
| `--alloc hoisted\|percall\|both` | How the C baseline manages `disort_state`. `hoisted` (default) matches what pydisort does and is the like-for-like comparison; `percall` matches `test_cdisort_09.c`. |
| `--mode radiance\|flux` | What both implementations compute. `radiance` (default) matches the C driver for Test 9. |
| `--verify-only` | Check agreement and exit. |
| `--tolerance` | Maximum relative difference accepted (default `1e-12` — both call the same solver, so agreement should be near machine precision). |
| `--rebuild`, `--builddir` | Force a rebuild / relocate the compiled baseline. |
| `--nwave`, `--threads`, `--repeat` | Problem sizes, thread count, timing repeats. |

### Result

Apple M5 Max (18 logical CPUs), pydisort 1.8.5, torch 2.10.0, Apple clang 21,
best of 3, radiance mode:

```
Speed-up over cdisort (hoisted)
   nwave         1 core       10 cores
       1          0.98x          0.98x
      10          1.00x          6.35x
     100          1.00x          7.80x
    1000          1.00x          8.17x
   10000          1.01x          8.34x

(same problem at 18 threads: 12.71x at nwave=1000, 13.25x at nwave=10000)
```

**The wrapper is free.** pydisort on one thread is 0.98–1.01x of cdisort
across four orders of magnitude of workload. That is the expected result — it
calls the same `c_disort` on the same inputs — but it is worth pinning down,
because it is exactly what a tensor-marshalling layer could plausibly get
wrong.

**Threading is where the gain is, and it is bounded by the core count.** On 10
threads the ratio saturates around **8.3x** (83% parallel efficiency); raising
the thread count to 18 on this machine takes it to 13.3x. There is nothing to
parallelize at one wavenumber, so the single- and multi-threaded lines meet
there and separate as the spectral dimension fills the cores. Quote this ratio
with the thread count and the CPU, or it does not mean anything.

## Against PythonicDISORT

```bash
pip install pydisort PythonicDISORT
python benchmarks/compare_pythonicdisort.py --verify-only   # seconds
python benchmarks/compare_pythonicdisort.py                 # default sweep
python benchmarks/compare_pythonicdisort.py \
    --nwave 1,10,100,1000 --threads 10 --compare-modes
```

| Option | Meaning |
| --- | --- |
| `--mode radiance\|flux` | What both solvers compute. `radiance` (default) matches the C driver for Test 9; `flux` has both compute fluxes only. |
| `--compare-modes` | Also time the other mode, to expose how much the ratio depends on that choice. |
| `--verify-only` | Check agreement and exit. |
| `--tolerance` | Maximum relative difference accepted (default `1e-9`). |
| `--nwave`, `--threads`, `--repeat` | As above. |

### Result

Same machine, PythonicDISORT 1.8, radiance mode, best of 1:

```
Speed-up over PythonicDISORT
   nwave      vs 1 core    vs 10 cores
       1          42.0x          42.4x
      10          41.7x         265.2x
     100          40.8x         314.8x
    1000          39.4x         332.1x
   10000          39.9x         334.7x
```

The single-core ratio is flat at about 40x — that is the interpreter overhead
and it does not depend on problem size. The multi-threaded ratio climbs while
the cores fill and then saturates near 335x by about 1000 wavenumbers.

That saturation is why the sweep above stops at 1000. The paper's figure
extends to 10000, where PythonicDISORT alone takes **552 s** on this machine
and closer to an hour on the M1 Max the paper used, for a ratio that has
already stopped moving. The script projects the runtime from one solve and
says so before it starts, so a long sweep is a choice rather than a surprise.
Add 10000 when reproducing the figure, not when checking a result.

> ⚠️ **Do not quote this ratio as a property of pydisort.** It is a ratio
> between two moving targets. PythonicDISORT's own performance has improved
> across releases (this table used 1.8), and the two codes do not scale
> identically across hardware, so the same measurement on a different machine
> or a different PythonicDISORT version can land a factor of two or three
> away. The durable, reproducible results are the two comparisons against
> cdisort: **1x on one thread** and **roughly an order of magnitude
> multi-threaded**. Re-run on your own hardware rather than citing these
> numbers.

## Getting the most out of pydisort

**Batch, do not loop.** Fill a `(nwave, ncol, nlyr, nprop)` tensor and make one
`forward` call rather than looping over wavelengths or columns. This is the
single most important thing for performance.

**Use `onlyfl` when you only need fluxes.** Skipping the radiance computation
is substantially cheaper.

**Control the thread count** with `torch.set_num_threads(n)`. The default is
usually reasonable, but when pydisort runs inside an already-parallel
application, oversubscription can cost more than it gains.
