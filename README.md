<!-- Logo ------------------------------------------->
<h4 align="center">
    <img src="docs/img/logo_tr_git.png" alt="Pydisort" width="340" style="display: block; margin: 0 auto">
</h4>

<!-- Subtitle --------------------------------------->
<p align="center">
  <i align="center">Empower Discrete Ordinates Radiative Transfer (DISORT) with Python 🚀</i>
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
<a href="">
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

DISORT (Discrete Ordinate Radiative Transfer) is a widely-used algorithm that calculates the scattering and absorption of radiation in a medium. The original DISORT algorithm was developed by Stamnes et al. in 1988 and was implemented in `FORTRAN`. Later, Timothy E. Dowling (1999) ported the algorithm to `C`, resulting in the widely-used implementation known as `cdisort`. The `cdisort` library is extensively utilized in atmospheric and remote sensing applications. Notably, it is an integral component of the `libRadtran` radiative transfer model, widely employed in atmospheric and remote sensing studies.

Building upon the aforementioned work, we have developed a `C++` wrapper for the `cdisort` library and subsequently created a `Python` package. The C++ wrapper serves two primary purposes: (1) providing a modern C++ interface for the `cdisort` library to facilitate future development involving DISORT, and (2) establishing the foundation for the Python package's bindings. The Python package, which is binded upon the C++ wrapper via `pybind11`, is designed to be user-friendly, making it easy to install and integrate into a diverse range of applications.

![](docs/img/rainbow.png)

## Table of Contents

- [Introduction](#introduction)
- [How to use](#how-to-use)
  - [For Python users](#for-python-users)
  - [For C++ developers](#for-c++-users)
    - [Check dependencies](#check-dependencies)
    - [Build and run the C++ wrapper](#build-and-run-the-c++-wrapper)
    - [Build and run the Python package](#build-and-run-the-python-package)
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
Detailed documentation of the function calls can be found at [pydisort documentation](https://pydisort.readthedocs.io/en/latest/).

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
pip3 install -r requirements.txt
pre-commit install
```

Installing `requirements.txt` also covers packages for the `pre-commit` hooks, which are very helpful if you'd like to make changes to the repository cloned. You could run the checks and lints manually using the following command to ensure that your changes are compliant with the industry standards:

```bash
pre-commit run --all-files
```

> 💡 Please feel free to add more checks and lints that suit your need to the `pre-commit` hooks. You could find more information about `pre-commit` [here](https://pre-commit.com/).

If you have no interest in adding or modifying features to the `pydisort` package, and just want to quickly build and run the C++ wrapper to your own use, you can follow the commands below:

```bash
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
cd tests
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

You can now run the test cases for the Python package using the following command:

```bash
$ python build/tests/test_attenuation.py
.
----------------------------------------------------------------------
Ran 1 test in 0.001s

OK
```

<div align="right"><a href="#table-of-contents"><img src="docs/img/top_green_small.png" width="32px"></div>

![](docs/img/rainbow.png)

## Contributing

[![Good first issues open](https://img.shields.io/github/issues/zoeyzyhu/pydisort/good%20first%20issue?label=good%20first%20issues&logo=git&logoColor=white&style=flat-square)](https://github.com/zoeyzyhu/pydisort/labels/good%20first%20issue)

Pull-Requests are welcomed. Fork repository, make changes, send us a pull request. We will review your changes and apply them to the main branch shortly, provided they don't violate our quality standards. Please read the [contribution guide](CONTRIBUTING.md) for details on the workflow, conventions, etc.

If you need to make changes to the `cdisort` library, please use patches to record your
modification. We keep a sole branch called `cidosrt_patches`, which contains the
cmake-built version of the `cdisort` library (v2.1.3) and all the patches that we have
applied to it. Please refer to the [patching guide](docs/README_patches.md) for more information.

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
