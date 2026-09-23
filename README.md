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
  - [For C++ developers](#for-c++-users)
    - [Check dependencies](#check-dependencies)
    - [Build and run the C++ wrapper](#build-and-run-the-c++-wrapper)
    - [Build and run the Python package](#build-and-run-the-python-package)
- [Examples](#examples)
- [Tests](#tests)
- [Benchmarks](#benchmarks)
- [Contributing](#contributing)
- [Issues?](#issues)

![](docs/img/rainbow.png)

## How to use

<!-- For Python users-------------------------------->

### <a id='for-python-users'><img src="docs/img/python.png" alt="Python" align=left width=24> For Python users</a>

We provide the `pydisort` library for Python users. The package can be installed using `pip`:

```bash
pip install pydisort
```

Prebuilt wheels are published for CPython 3.10–3.14, and `pip` pulls in a compatible `torch` automatically. `pydisort` is a compiled PyTorch extension, so if `pip` falls back to building from source (for example on a platform with no matching wheel), `torch` must be importable at build time. Install it first and disable build isolation so the build can see it:

```bash
pip install 'torch==2.10.0'
pip install pydisort --no-build-isolation
```

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
number of phase moments (nmom), and number of phases (nphase).
Usually, `nlyr`, `nstr`, `nmom` and `nphase` are the same.

The example above sets the number of layers to 4, number of streams to 4.
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
tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).unsqueeze(-1)
```

- Step 5. Run radiative transfer and get intensity result.

A `ds` object is constructed as if it is one layer of a Neural Network model.
The core function is the forward function, which takes the optical properties and radiation boundary conditions as input.
Radiation boundary conditions are passed as keyword arguments.
The dimensions will be automatically expanded to account for degenerate dimensions such as wave and column.
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

[//]: <> (!!Do not remove the following line, which is used for dividing the content)

#

<!-- For C++ developers------------------------------>

### <a id='for-c++-users'><img src="docs/img/cpp.png" alt="C++" align=left width=24> For C++ developers</a>

#### <a id='check-dependencies'> 🔻 Check dependences</a>

This repository supports both the Linux and MacOS operating systems. The following dependencies are required for building the C++ wrapper:

- `cmake` (version >= 3.16)
- `g++` (version >= 7.5.0)
- `python3` (version >= 3.9)

You could check the versions of these dependencies using the following commands:

```bash
 cmake --version
 g++ --version
 python3 --version
```

If you need to install these dependencies, use the following commands (replacing `apt-get` with your package manager if you are not using Ubuntu):

```bash
 sudo apt-get install cmake
 sudo apt-get install g++
 sudo apt-get install python3
```

#### <a id='build-and-run-the-c++-wrapper'> 🔻 Build and run the C++ wrapper</a>

You could fork or clone this repository to your local machine.

```bash
git clone https://github.com/zoeyzyhu/pydisort.git
```

Before building the package, you need to install the dependencies for the `pydisort` package and the pre-commit hooks. We recommend that you use a virtual environment for Python to install the `pydisort` package and the dependencies. You could use the following commands to create a virtual environment, install the dependencies, and build the package:

```bash
cd pydisort
python3 -m venv env
source env/bin/activate  # Make sure you are in the virtual environment

# Tools for developing against the repository
pip3 install pre-commit pytest
pre-commit install
```

`pre-commit` runs the checks and lints configured in `.pre-commit-config.yaml`, which are very helpful if you'd like to make changes to the repository cloned. You could run them manually using the following command to ensure that your changes are compliant with the industry standards:

```bash
pre-commit run --all-files
```

> 💡 Please feel free to add more checks and lints that suit your need to the `pre-commit` hooks. You could find more information about `pre-commit` [here](https://pre-commit.com/).

If you have no interest in adding or modifying features to the `pydisort` package, and just want to quickly build and run the C++ wrapper to your own use, you can follow the commands below:

The C++ build resolves PyTorch through CMake (`find_package(Torch REQUIRED)`), so `torch` has to be importable in the active environment before you configure. `pip install .` in the next section pulls it in on its own, but a standalone `cmake ..` does not.

```bash
pip3 install 'torch==2.10.0'  # only when building the C++ side on its own

mkdir build
cd build
cmake ..
make
```

After the build is complete, you can run the C++ wrapper using the following commands:

```bash
# Assume that you are still in the build/ directory
cd tests

# Run the test
./test_disort.release
```

#### <a id='build-and-run-the-python-package'> 🔻 Build and run the Python package</a>

If you follow the steps in the previous section, you will have a C++ wrapper that can be used by Python, and a Python packaged called `pydisort`, which has been binded via `pybind11`. You could simply install and test the Python package using the following command:

```bash
# Assume that you are still in the build/bin/ directory
# Install the Python package
cd ../..  # Go back to the root directory
pip install .
```

You can now run the test cases for the Python package with `pytest`:

```bash
$ pytest tests/
======================================= test session starts ========================================
platform darwin -- Python 3.12.13, pytest-9.1.1, pluggy-1.6.0
collected 188 items

tests/cuda/test_cpu_cuda_agreement.py ssssssssssssssssssssssssssssssssssssssssssssssssssssss [ 28%]
ssssssssssssss                                                                               [ 36%]
tests/cuda/test_fast_flux_routing.py ssssssssssssssssssssss                                  [ 47%]
tests/reference/test_problem_01_isotropic.py ......                                          [ 51%]
tests/reference/test_problem_02_rayleigh.py ....                                             [ 53%]
tests/reference/test_problem_03_henyey_greenstein.py ...                                     [ 54%]
tests/reference/test_problem_06_lambertian_surface.py ....                                   [ 56%]
tests/reference/test_problem_09_inhomogeneous.py .....                                       [ 59%]
tests/reference/test_problem_10_user_vs_quadrature.py ...                                    [ 61%]
tests/test_attenuation.py .                                                                  [ 61%]
tests/test_batching.py .........                                                             [ 66%]
tests/test_examples.py ....                                                                  [ 68%]
tests/test_input_validation.py ...........                                                   [ 74%]
tests/test_output_accessors.py .........                                                     [ 79%]
tests/test_scattering_moments.py .......................................                     [100%]

================================== 98 passed, 90 skipped in 0.45s ==================================
```

The 90 skips are the CUDA agreement checks in [`tests/cuda/`](tests/cuda/), which run only where a GPU is available. Add `-v` to list the individual test cases instead of one line per file.

<div align="right"><a href="#table-of-contents"><img src="docs/img/top_green_small.png" width="32px"></div>

![](docs/img/rainbow.png)

## Examples

The [`examples/`](examples/) directory contains four complete, runnable
calculations that build from the simplest possible DISORT problem to a
research-level analysis. Each one is standalone, prints its results, and ends
with assertions against an analytic solution or a conservation law, so running
one is also a way to verify your installation.

| Example | What it covers |
| --- | --- |
| [`example_01_beam_attenuation.py`](examples/example_01_beam_attenuation.py) | The basic workflow; beam attenuation, validated against the Beer–Lambert law to machine precision. |
| [`example_02_thermal_emission.py`](examples/example_02_thermal_emission.py) | Thermal emission and longwave cooling rates for an Earth-like column; batching over the **spectral** axis. |
| [`example_03_aerosol_scattering.py`](examples/example_03_aerosol_scattering.py) | Multiple scattering and radiances; batching over the **column** axis to build a remote-sensing lookup table. |
| [`example_04_two_stream_validation.py`](examples/example_04_two_stream_validation.py) | **A real-world analysis problem.** Using pydisort as the multi-stream reference to measure the error of a fast two-stream solver. |

```bash
pip install pydisort
python examples/example_01_beam_attenuation.py
```

Example 4 is the workflow behind every fast radiation scheme: two-stream
solvers are cheap enough for climate models and operational retrievals, and
DISORT is the multi-stream reference you check them against. It solves twelve
official DISORT flux-test cases, reproduces the published benchmark fluxes to
**0.0005%**, then re-solves them at two streams to show where the approximation
breaks down, on near-zero fluxes and on forward-peaked phase functions. This is
the same check [`py2sess`](https://github.com/happysky19/py2sess) uses to
validate its own two-stream solver. See
[`examples/README.md`](examples/README.md) for details.

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

> 💡 Older instructions exclude Test Problem 9 with `-E test_disort_09`,
> because that module used to carry a performance driver in its `__main__`
> block. The timing code now lives in [`benchmarks/`](benchmarks/README.md)
> and the exclusion would now only skip coverage.

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

# the interpreted/compiled gap, against the pure-Python PythonicDISORT
python benchmarks/compare_pythonicdisort.py --verify-only
python benchmarks/compare_pythonicdisort.py --threads 10
```

On one thread pydisort matches cdisort exactly, so the wrapper costs nothing;
with ten threads it is roughly an order of magnitude faster. The pure-Python
PythonicDISORT is slower still, though by how much depends on the machine and
on its version, so that ratio is worth measuring rather than quoting.

Both import the problem definition from `benchmarks/testproblem09.py`, so the
configuration being timed is written down once rather than copied. See
[`benchmarks/README.md`](benchmarks/README.md).

<div align="right"><a href="#table-of-contents"><img src="docs/img/top_green_small.png" width="32px"></div>

![](docs/img/rainbow.png)

## Contributing

[![Good first issues open](https://img.shields.io/github/issues/zoeyzyhu/pydisort/good%20first%20issue?label=good%20first%20issues&logo=git&logoColor=white&style=flat-square)](https://github.com/zoeyzyhu/pydisort/labels/good%20first%20issue)

Pull-Requests are welcomed. Fork repository, make changes, send us a pull request. We will review your changes and apply them to the main branch shortly, provided they don't violate our quality standards. Please read the [contribution guide](CONTRIBUTING.md) for details on the workflow, conventions, etc.

If you need to make changes to the `cdisort` library, please use patches to record your
modification. We keep a sole branch called `cidosrt_patches`, which contains the
cmake-built version of the `cdisort` library (v2.1.3) and all the patches that we have
applied to it. Please refer to the [`cdisort_patches` branch](https://github.com/zoeyzyhu/pydisort/tree/cdisort_patches) for more information.

If you need to include more libraries to the `Disort` wrapper, please use the `CMakeLists.txt` file to add them. You could find more information about the cmake build system [here](https://cmake.org/cmake/help/latest/guide/tutorial/index.html).

If you need to make changes to the `pydisort` package, please use the `pybind11` library to bind the C++ wrapper to Python, expose the functions and classes to Python, and add more test cases to the `pydisort` package. You could find more information about the `pybind11` library [here](https://pybind11.readthedocs.io/en/stable/).

For more information to assist your development, please refer to the `docs/` folder in this repository.

<div align="right"><a href="#table-of-contents"><img src="docs/img/top_green_small.png" width="32px"></div>

![](docs/img/rainbow.png)

## Issues?

This repository is maintained actively, so if you face any issue please <a href="https://github.com/zoeyzyhu/pydisort/issues">raise an issue</a>.

Not sure where to start? Join our discord and we will help you get started!

<a href="https://discord.gg/ZKBZg5K2"><img src="docs/img/discord.png" width="150"/></a>
&nbsp;&nbsp; <a target="_blank" href="https://bmc.link/zoeyzyhu"><img src="docs/img/bmc_white.png" alt="Buy me a coffee" width="170"/></a>
