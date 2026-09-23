<!-- Logo ------------------------------------------->
<h4 align="center">
    <img src="docs/img/logo_tr_git.png" alt="Pydisort" width="340" style="display: block; margin: 0 auto">
</h4>

<!-- Subtitle --------------------------------------->
<p align="center">
  <i align="center">Superpowered Radiative Transfer: Fast, Scalable, User-Friendly 🚀</i>
</p>

<!-- primary badges --------------------------------->
<p align="center">
<!---
<a href="https://www.codacy.com/gh/ankitwasankar/mftool-java/dashboard?utm_source=github.com&utm_medium=referral&utm_content=ankitwasankar/mftool-java&utm_campaign=Badge_Coverage">
  <img alt="Tests coverage"
    src="https://app.codacy.com/project/badge/Coverage/0054db87ea0f426599c3a30b39291388?style=flat-square"/>
</a>
<a href="https://codecov.io/gh/{{REPOSITORY}}">
  <img alt="Tests coverage"
    src="https://codecov.io/gh/{{REPOSITORY}}/branch/main/graph/badge.svg?style=flat-square?"/>
</a>
-->
<a href="https://github.com/zoeyzyhu/pydisort/actions">
  <img alt="GitHub Workflow Status"
    src="https://img.shields.io/github/actions/workflow/status/zoeyzyhu/pydisort/ci.yml?style=flat-square&logo=github"/>
</a>
<a href="https://pydisort.readthedocs.io/en/latest/">
  <img alt="Documentation Status"
    src="https://app.readthedocs.org/projects/pydisort/badge/?version=latest&style=flat-square"/>
</a>
<!---
<a href="https://codecov.io/gh/{{REPOSITORY}}">
  <img alt="Codacy coverage"
    src="https://img.shields.io/codacy/coverage/pydisort?style=flat-square&logo=codecov"/>
</a>
-->
<a href="https://github.com/zoeyzyhu/pydisort/issues">
  <img alt="GitHub issues"
    src="https://img.shields.io/github/issues/zoeyzyhu/pydisort?style=flat-square&logo=git"/>
</a>
<a href="https://github.com/zoeyzyhu/pydisort/releases">
  <img alt="GitHub release (latest by date)"
    src="https://img.shields.io/github/v/release/zoeyzyhu/pydisort?style=flat-square&logo=buffer"/>
</a>
<br>
<a href="https://github.com/pre-commit/pre-commit">
  <img alt="pre-commit"
    src="https://img.shields.io/badge/pre--commit-enabled-brightgreen?style=flat-square&logo=pre-commit"/>
</a>
<a href="http://makeapullrequest.com">
  <img alt="pull-request"
    src="https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square&logo=git"/>
</a>
<a href="https://opensource.org/licenses/">
  <img alt="license"
    src="https://img.shields.io/badge/License-GPL-yellow.svg?style=flat-square&logo=gnu"/>
</a>
<a href="https://img.shields.io/badge/OS-Linux%2C%20MacOS-orange">
  <img alt="os"
    src="https://img.shields.io/badge/OS-linux%2C%20mac-orange?style=flat-square&logo=linux"/>
</a>
</p>

<!-- description ------------------------------------>

<p align="center">
  This repository contains a Python package and a C++ wrapper for the <code>cdisort</code> library, which is a C implementation of the DISORT radiative transfer model.
</p>

<!-- Navigation-------------------------------------->
<p align="center">
<a href="#introduction">Introduction</a> &nbsp;&bull;&nbsp;
<a href="#how-to-use">How to use</a> &nbsp;&bull;&nbsp;
<a href="#examples">Examples</a> &nbsp;&bull;&nbsp;
<a href="https://pydisort.readthedocs.io/en/latest/">Documentation</a> &nbsp;&bull;&nbsp;
<a href="#contributing">Contributing</a> &nbsp;&bull;&nbsp;
<a href="#issues">Issues?</a>
</p>

<br/><br/>

<!-- Body ------------------------------------------->

## Introduction

DISORT (Discrete Ordinate Radiative Transfer) is a widely-used algorithm that calculates the scattering and absorption of radiation in a medium. The `pydisort` project provides both a high-level Python API and a C++ layer to the well-tested C implementation of DISORT, originally developed in Fortran (Stamnes et al. 1988) and later ported to C as `cdisort` by Timothy E. Dowling, which is a critical component of `libRadTran`.

To support Python integration, the C code was encapsulated in C++ classes. The C++ wrapper serves two primary purposes: (1) providing a modern C++ interface for the `cdisort` library to facilitate future development involving DISORT, and (2) establishing the foundation for the Python package's bindings. The Python package, which is binded upon the C++ wrapper via `pybind11`, is designed to be user-friendly, making it easy to install and integrate into a diverse range of applications.

For efficient memory management and potential GPU acceleration, `pydisort` leverages `PyTorch` tensors, paving the way for future applications in machine learning and large-scale parallel computation.


![](docs/img/rainbow.png)

## Table of Contents

- [Introduction](#introduction)
- [How to use](#how-to-use)
  - [For Python users](#for-python-users)
- [Examples](#examples)
- [Documentation](#documentation)
- [Tests](#tests)
- [Benchmarks](#benchmarks)
- [For C++ developers](#for-c++-users)
  - [Check dependencies](#check-dependencies)
  - [Build and run the C++ wrapper](#build-and-run-the-c++-wrapper)
  - [Build and run the Python package](#build-and-run-the-python-package)
- [Contributing](#contributing)
- [Citation](#citation)
- [Issues?](#issues)

![](docs/img/rainbow.png)

## How to use

<!-- For Python users-------------------------------->

### <a id='for-python-users'><img src="docs/img/python.png" alt="Python" align=left width=24> For Python users</a>

We provide the `pydisort` library for Python users. The package can be installed using `pip`:

```bash
pip install pydisort
```

Prebuilt wheels cover CPython 3.10–3.14 on Linux x86-64
(glibc 2.28 or newer with PyTorch) and Apple Silicon macOS 15 or newer.
`pip` installs the required PyTorch version automatically.
The release workflow publishes wheels only, so there is no automatic source-build
fallback when a wheel does not match. See the
[installation guide](https://pydisort.readthedocs.io/en/latest/installation.html)
for the complete checkout → CMake → pip source-build procedure.

Here is a step-by-step tutorial of how to use the pydisort package:

- Step 1. Importing the module.

`DisortOptions` is a class that contains the options for configuring the DISORT model
`Disort` is the main class for running the DISORT model

```python
import pydisort
from pydisort import DisortOptions, Disort
```

- Step 2. Configure dimensions and options.

DISORT solves plane-parallel radiative transfer problems in a 1D atmosphere
The dimensions are number of layers (nlyr), number of streams (nstr),
number of phase-function moments (nmom), and number of explicit phase-function
grid points (nphase). Layer count is independent of angular resolution;
these values happen to be equal in this small example. `nphase` is not the
number of output azimuths.

The example below sets the number of layers to 4, number of streams to 4.
Radiation flags are packed in a string and passed to the flags function.
See later for more details on the flags.

```python
op = DisortOptions().flags("onlyfl,lamber")
op.ds().nlyr = 4
op.ds().nstr = 4
op.ds().nmom = 4
op.ds().nphase = 4
```

- Step 3. Construct the Disort object based on the options.

`ds` is the main object for running the DISORT model.
It is constructed using the options defined in the previous step.
Internal memory is allocated for the DISORT model.

```python
ds = Disort(op)
```

- Step 4. Set up optical properties

`pydisort` uses torch tensors to store the optical properties.
The statement above sets the layer optical thickness from top to bottom.
The last dimension of the tau tensor is the number of optical properties,
in the order of optical thickness, single scattering albedo, and moments of scattering phase function.
The second to the last dimension of tau is the number of layers,
which must be the same as the number of layers in the DisortOptions object.

```python
import torch
torch.set_default_dtype(torch.float64)
tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).unsqueeze(-1)
```

- Step 5. Run radiative transfer and get fluxes.

A `ds` object is constructed as if it is one layer of a Neural Network model.
The core function is the forward function, which takes the optical properties and radiation boundary conditions as input.
Radiation boundary conditions are passed as keyword arguments.
Missing leading dimensions are inserted as singleton axes; inputs are not
automatically repeated across larger batches. Shapes must match the configured
wave and column counts. See the
[shape rules](https://pydisort.readthedocs.io/en/latest/usage.html#the-two-batch-dimensions).
The output is the upward and downward fluxes at each level of the atmosphere.

```python
ds.forward(tau, fbeam=torch.tensor([3.14159]))
```

The result of the example above should be:
```python
tensor([[[[0.0000, 3.1416],
          [0.0000, 2.8426],
          [0.0000, 2.3273],
          [0.0000, 1.7241],
          [0.0000, 1.1557]]]])
```

This is 4D tensor with dimensions (wavelengths, columns, levels, 2).
In the last dimension, the first element is the upward flux and the second element is the downward flux.
Number of levels is one more than the number of layers.

Please note that this is a generic tutorial and you would need to adapt this to your specific use-case.
Detailed documentation of the function calls can be found at [pydisort documentation](https://pydisort.readthedocs.io/en/latest/), and complete worked calculations are in [`examples/`](examples/).

> 💡 We keep the parameters consistent to the original `DISORT` library, so you can refer to the [DISORT documentation](cdisort213/DISORT2.doc) for more information such as input/out variables, flags, model usage and caveats.

> 💡 One important point to note is that the `pydisort` library assumes that the provided arrays (optical thickness, single scattering albedo, boundary condition etc.) have strict dimension requirements because operations are batched over wavenumbers and columns.
It throws exceptions if incompatible dimensions are provided. So, ensure that you are providing data in the right dimensions to avoid any runtime errors.

<div align="right"><a href="#table-of-contents"><img src="docs/img/top_green_small.png" width="32px"></div>

![](docs/img/rainbow.png)

## Examples

The [`examples/`](examples/) directory contains four complete, runnable
calculations that build from the simplest possible DISORT problem to a
research-level analysis. Each is standalone, prints its results, and ends
with assertions against reference values, analytic limits, conservation laws
or internal consistency relations. Running them checks those configurations.

| Example | What it covers |
| --- | --- |
| [`example_01_beam_attenuation.py`](examples/example_01_beam_attenuation.py) | The basic workflow; beam attenuation, validated against the Beer-Lambert law to machine precision. |
| [`example_02_thermal_emission.py`](examples/example_02_thermal_emission.py) | Thermal emission and longwave cooling rates for an Earth-like column; batching over the **spectral** axis. |
| [`example_03_aerosol_scattering.py`](examples/example_03_aerosol_scattering.py) | Multiple scattering and radiances; batching over the **column** axis to build a remote-sensing lookup table. |
| [`example_04_two_stream_validation.py`](examples/example_04_two_stream_validation.py) | **A real-world analysis problem.** Assessing the two-stream DISORT approximation against published fluxes and exploring stream resolution. |

```bash
pip install pydisort
python examples/example_01_beam_attenuation.py
```

Example 4 solves twelve official DISORT flux-test cases, reproduces selected
published benchmark fluxes to about **0.0005%** at 16 streams, and inspects
discrepancies as the stream count changes. It runs pydisort only, not an
external solver such as py2sess. Its percentiles cannot rank different solvers
without matching their cases, output conventions and scoring rules. See
[`examples/README.md`](examples/README.md) for details.

<div align="right"><a href="#table-of-contents"><img src="docs/img/top_green_small.png" width="32px"></div>

![](docs/img/rainbow.png)

## Documentation

The full documentation is hosted at **[pydisort.readthedocs.io](https://pydisort.readthedocs.io/en/latest/)**.

| Page | Contents |
| --- | --- |
| [Installation and Quickstart](https://pydisort.readthedocs.io/en/latest/installation.html) | Install, verify the install, and a minimal working program. |
| [Statement of Need](https://pydisort.readthedocs.io/en/latest/statement_of_need.html) | What pydisort solves, and who it is for. |
| [User Guide](https://pydisort.readthedocs.io/en/latest/usage.html) | Input/output tensor layouts, batching and singleton-dimension rules. |
| [API Reference](https://pydisort.readthedocs.io/en/latest/api.html) | Every class, method and flag. |
| [Automated Tests](https://pydisort.readthedocs.io/en/latest/testing.html) | How correctness is established, and how to run and extend the suite. |
| [Performance](https://pydisort.readthedocs.io/en/latest/benchmarks.html) | Timings against cdisort and PythonicDISORT, and how to reproduce them. |

<div align="right"><a href="#table-of-contents"><img src="docs/img/top_green_small.png" width="32px"></div>

![](docs/img/rainbow.png)

## Tests

The Python test suite only needs an installed `pydisort`:

```bash
pip install pydisort pytest
pytest tests/ -v
```

The full suite, including the C and C++ tests, is driven by CTest:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Correctness is established three ways: against **published DISORT reference
values** (`tests/reference/` ports six of the fourteen DISORT test problems), against **analytic solutions**, and
against **conservation laws**. See the
[testing documentation](https://pydisort.readthedocs.io/en/latest/testing.html)
for the full description.

<div align="right"><a href="#table-of-contents"><img src="docs/img/top_green_small.png" width="32px"></div>

![](docs/img/rainbow.png)

## Benchmarks

Spectral resolution is the dimension that makes radiative transfer expensive,
and it is the dimension pydisort parallelizes over. Two scripts measure how
runtime grows along it, on DISORT Test Problem 9 with 32 streams and 100
layers. Both **verify that the two implementations agree on the fluxes before
reporting any timing**, and exit non-zero if they do not.

```bash
# what the Python wrapper costs; builds its own C baseline, needs no cmake
python benchmarks/compare_cdisort.py --verify-only
python benchmarks/compare_cdisort.py --alloc both

# end-to-end solver performance against PythonicDISORT
python -m pip install PythonicDISORT threadpoolctl
python benchmarks/compare_pythonicdisort.py --verify-only
python benchmarks/compare_pythonicdisort.py --threads 10
```

Single-threaded performance closely matches cdisort on this workload;
with ten threads it is roughly an order of magnitude faster. The pure-Python
PythonicDISORT is slower still, though by how much depends on the machine and
on its version, so that ratio is worth measuring rather than quoting.

Both import the problem definition from `benchmarks/testproblem09.py`, so the
configuration being timed is written down once rather than copied. See
[`benchmarks/README.md`](benchmarks/README.md).

<div align="right"><a href="#table-of-contents"><img src="docs/img/top_green_small.png" width="32px"></div>

![](docs/img/rainbow.png)

<!-- For C++ developers------------------------------>

## <a id='for-c++-users'><img src="docs/img/cpp.png" alt="C++" align=left width=24> For C++ developers</a>

### <a id='check-dependencies'>Build requirements</a>

Use CMake 3.20 or newer, a C++17 compiler (GCC 9 or newer on Linux, or
compatible Apple Clang on macOS), and a supported CPython version.
PyTorch must be installed in the active environment before running CMake.

### <a id='build-and-run-the-c++-wrapper'>Build the C++ wrapper</a>

From a clean checkout and an activated virtual environment:

```bash
python -m pip install 'torch==2.10.0' numpy pytest 'cmake>=3.20' ninja setuptools 'setuptools-scm>=8' wheel
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build --parallel
```

### <a id='build-and-run-the-python-package'>Build the Python bindings and validate</a>

Run from the repository root, after the CMake build:

```bash
python -m pip install --no-build-isolation .
python -m pytest tests/ -v -rs
ctest --test-dir build --output-on-failure
```

The pip step links the C++ library from `build/lib`; it does not run CMake.
`-v` lists individual test cases and `-rs` explains skips. CUDA checks are
skipped when their GPU requirements are not met; counts can change as tests
are added. See the [contributor guide](CONTRIBUTING.md) for development tools
and documentation checks.

<div align="right"><a href="#table-of-contents"><img src="docs/img/top_green_small.png" width="32px"></div>

![](docs/img/rainbow.png)

## Contributing

[![Good first issues open](https://img.shields.io/github/issues/zoeyzyhu/pydisort/good%20first%20issue?label=good%20first%20issues&logo=git&logoColor=white&style=flat-square)](https://github.com/zoeyzyhu/pydisort/labels/good%20first%20issue)

Pull-Requests are welcomed. Fork repository, make changes, send us a pull request. We will review your changes and apply them to the main branch shortly, provided they don't violate our quality standards. Please read the [contribution guide](CONTRIBUTING.md) for details on the workflow, conventions, etc.

If you need to make changes to the `cdisort` library, please use patches to record your
modification. We keep a sole branch called `cdisort_patches`, which contains the
cmake-built version of the `cdisort` library (v2.1.3) and all the patches that we have
applied to it. Please refer to the [`cdisort_patches` branch](https://github.com/zoeyzyhu/pydisort/tree/cdisort_patches) for more information.

If you need to include more libraries to the `Disort` wrapper, please use the `CMakeLists.txt` file to add them. You could find more information about the cmake build system [here](https://cmake.org/cmake/help/latest/guide/tutorial/index.html).

If you need to make changes to the `pydisort` package, please use the `pybind11` library to bind the C++ wrapper to Python, expose the functions and classes to Python, and add more test cases to the `pydisort` package. You could find more information about the `pybind11` library [here](https://pybind11.readthedocs.io/en/stable/).

For more information to assist your development, please refer to the `docs/` folder in this repository.

<div align="right"><a href="#table-of-contents"><img src="docs/img/top_green_small.png" width="32px"></div>

![](docs/img/rainbow.png)

## Citation

If you use `pydisort` in your work, please cite it. Machine-readable metadata lives in [`CITATION.cff`](CITATION.cff), which GitHub renders as BibTeX or APA through the **Cite this repository** button in the sidebar.

<div align="right"><a href="#table-of-contents"><img src="docs/img/top_green_small.png" width="32px"></div>

![](docs/img/rainbow.png)

## Issues?

This repository is maintained actively, so if you face any issue please <a href="https://github.com/zoeyzyhu/pydisort/issues">raise an issue</a>.

Not sure where to start? Join our discord and we will help you get started!

<a href="https://discord.gg/ZKBZg5K2"><img src="docs/img/discord.png" width="150"/></a>
&nbsp;&nbsp; <a target="_blank" href="https://bmc.link/zoeyzyhu"><img src="docs/img/bmc_white.png" alt="Buy me a coffee" width="170"/></a>
