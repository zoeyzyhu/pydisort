<!-- Logo ------------------------------------------->
<h4 align="center">
    <img src="img/logo.png" alt="Pydisort" width="300" style="display: block; margin: 0 auto">
</h4>

<!-- Subtitle --------------------------------------->
<p align="center">
  <i align="center">Empower Discrete Ordinates Radiative Transfer (DISORT) with Python 🚀</i>
</p>

<!-- primary badges --------------------------------->
<p align="center">
<a href="https://github.com/zoeyzyhu/actions/workflows/main.yml">
  <img alt="GitHub Workflow Status"
    src="https://img.shields.io/github/actions/workflow/status/zoeyzyhu/pydisort/main.yml?style=flat-square&logo=github">
</a>
<a href="https://www.codacy.com/gh/ankitwasankar/mftool-java/dashboard?utm_source=github.com&utm_medium=referral&utm_content=ankitwasankar/mftool-java&utm_campaign=Badge_Coverage">
  <img alt="Tests coverage"
    src="https://app.codacy.com/project/badge/Coverage/0054db87ea0f426599c3a30b39291388?style=flat-square" />
</a>
<a href="https://codecov.io/gh/{{REPOSITORY}}">
  <img alt="Tests coverage"
    src="https://codecov.io/gh/{{REPOSITORY}}/branch/main/graph/badge.svg?style=flat-square?style=flat-square" />
</a>
<a href="https://github.com/zoeyzyhu/pydisort/issues">
  <img alt="GitHub issues"
    src="https://img.shields.io/github/issues/zoeyzyhu/pydisort?style=flat-square&logo=git"">
</a>
<a href="https://github.com/zoeyzyhu/pydisort/releases">
  <img alt="GitHub release (latest by date)"
    src="https://img.shields.io/github/v/release/zoeyzyhu/pydisort?style=flat-square&logo=buffer">
</a>
<br>
<a href="https://github.com/pre-commit/pre-commit">
  <img alt="pre-commit"
    src="https://img.shields.io/badge/pre--commit-enabled-brightgreen?style=flat-square&logo=pre-commit">
</a>
<a href="http://makeapullrequest.com">
  <img alt="pre-commit"
    src="https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square&logo=git">
</a>
<a href="https://opensource.org/licenses/">
  <img alt="license"
    src="https://img.shields.io/badge/License-GPL-yellow.svg?style=flat-square&logo=gnu" />
</a>
<a href="https://img.shields.io/badge/OS-Linux%2C%20MacOS-orange">
  <img alt="os"
    src="https://img.shields.io/badge/OS-linux%2C%20mac-orange?style=flat-square&logo=linux" />
</a>
</p>

<!-- description ------------------------------------>

<p align="center">
  This repository contains a Python package and a C++ wrapper for the <code>cdisort</code> library, which is a C implementation of the DISORT radiative transfer model.
</p>

<!-- Navigation-------------------------------------->
<p align="center">
<a href="#introduction">Introduction</a> &nbsp;&bull;&nbsp;
<a href="#usage">Usage</a> &nbsp;&bull;&nbsp;
<a href="#contribution">Contribution</a> &nbsp;&bull;&nbsp;
<a href="#issues">Issues?</a>
</p>

<br/><br/>

<!-- Body ------------------------------------------->

## Introduction

DISORT (Discrete Ordinate Radiative Transfer) is a widely-used algorithm that calculates the scattering and absorption of radiation in a medium. The original DISORT algorithm was developed by Stamnes et al. in 1988 and was implemented in `FORTRAN`. Later, Timothy E. Dowling (1999) ported the algorithm to `C`, resulting in the widely-used implementation known as `cdisort`. The `cdisort` library is extensively utilized in atmospheric and remote sensing applications. Notably, it is an integral component of the `libRadtran` radiative transfer model, widely employed in atmospheric and remote sensing studies.

Building upon the aforementioned work, we have developed a `C++` wrapper for the `cdisort` library and subsequently created a `Python` package. The C++ wrapper serves two primary purposes: (1) providing a modern C++ interface for the `cdisort` library to facilitate future development involving DISORT, and (2) establishing the foundation for the Python package's bindings. The Python package, which is binded upon the C++ wrapper via `pybind11`, is designed to be user-friendly, making it easy to install and integrate into a diverse range of applications.

![-----------------------------------------------------](img/rainbow.png)

## Table of Contents

- [Introduction](#introduction)
- [Usage](#usage)
  - [For Python users](#for-python-users)
  - [For C++ developers](#for-c++-users)

![-----------------------------------------------------](img/rainbow.png)

## Usage

<!-- For Python users-------------------------------->

### <a id='for-python-users'><picture><img src="img/python.svg" alt="Python" align=left width=24></picture> For Python users</a>

We provide the `pydisort` library for Python users. The package can be installed using `pip`:

```bash
pip install pydisort
```

Here is a step-by-step tutorial of how to use the pydisort package:

- Step 1. Importing the module:

```python
import pydisort
import numpy as np
```

- Step 2. Create an instance of the disort class:

```python
# Let's assume you have a file named 'input.toml' which has the
# required data for initializing the 'disort' class.
disort_instance = pydisort.disort.from_file('input.toml')
```

- Step 3. Set the properties of your disort model:

```python
# Let's assume you have the following arrays for setting the disort properties
optical_depth = np.array([1.0, 2.0, 3.0])
single_scattering_albedo = np.array([0.7, 0.8, 0.9])
level_temperature = np.array([300.0, 200.0, 100.0])

disort_instance.set_optical_depth(optical_depth)
disort_instance.set_single_scattering_albedo(single_scattering_albedo)
disort_instance.set_level_temperature(level_temperature)
```

- Step 4. Set more specific options, such as flags or intensity dimensions:

```python
flags = {"flag_1": True, "flag_2": False}
disort_instance.set_flags(flags)
disort_instance.set_intensity_dimension(1, 1, 1)
```

- Step 5. Run the disort computation:

```python
disort_instance.run()
```

- Step 6. After running the disort computation, you can get the computed flux and intensity:

```python
flux = disort_instance.get_flux()
intensity = disort_instance.get_intensity()
```

Please note that this is a generic tutorial and you would need to adapt this to your specific use-case. For example, you might need to provide your own data file in `from_file` function or fill the numpy arrays `optical_depth`, `single_scattering_albedo`, and `level_temperature` according to your requirements.

> 💡 One important point to note is that the `pydisort` library assumes that the provided arrays (optical depth, single scattering albedo, etc.) are in the numpy format and it throws exceptions if incompatible data types are provided. So, ensure that you are providing data in the right format to avoid any runtime errors.

<div align="right"><a href="#table-of-contents"><img src="img/top.png" width="32"></div>

[//]: <> (!!Do not remove the following line, which is used for dividing the content)

#

<!-- For C++ developers------------------------------>

### <a id='for-c++-users'><picture><img src="img/cpp.svg" alt="C++" align=left width=24></picture> For C++ developers</a>

#### 🔻 Check dependences

This repository supports both the Linux and MacOS operating systems. The following dependencies are required for building the C++ wrapper:

- `cmake` (version >= 3.20)
- `g++` (version >= 7.5.0)
- `python3` (version >= 3.6)

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

#### 🔻 Build and run the C++ wrapper

You could fork or clone this repository to your local machine.

```bash
git clone https://github.com/zoeyzyhu/pydisort.git
```

If you have no interest in using or adding features to the `pydisort` package, and just want to quickly build and run the C++ wrapper to your own use, you can follow the commands below:

```bash
mkdir build
cd build
cmake ..
make
```

After the build is complete, you can run the C++ wrapper using the following commands:

```bash
# Assume that you are still in the build/ directory
cd bin

# Run the C++ wrapper
./test_cppdisort.release

# If you're interested in `cdisort`, run the test provided by Dowling
./test_cdisort.release
```

#### 🔻 Build and run the Python package

If you follow the steps in the previous section, you will have a C++ wrapper that can be used by Python, and a Python packaged called `pydisort`, which has been binded via `pybind11`. You could simply install and test the Python package using the following command:

```bash
# Assume that you are still in the build/bin/ directory
# Install the Python package
cd ..
make install

# Run the test cases
cd bin
python3 test_isotropic_scattering.py
```

❗However, the above steps will put `pydisort` in the system path of Python, which might be inaccessible especially if you are working on a shared server. We recommend that you use a virtual environment for Python to install the `pydisort` package, which will also keep your system path clean even if you have access to it.

In this recommended approach, we need to set up the virtual environment before the building process. You could use the following commands to create a virtual environment, build and install the `pydisort` package in it:

```bash
# Set up Python virtual environment and cppcheck
./1.setup.sh

# Activate the virtual environment
source env/bin/activate

# Install dependencies for `pydisort` and pre-commit hooks
./2.install.sh

# Build the C++ wrapper and Python package
./3.build.sh

# Run test cases for C++ wrapper and Python package
./4.test.sh
```

The steps above will also install packages for the `pre-commit` hooks, which are very helpful if you'd like to make changes to the repository cloned. You could run the checks and lints manually using the following command to ensure that your changes are compliant with the industry standards:

```bash
pre-commit run --all-files
```

> 💡 Please feel free to add more checks and lints that suit your need to the `pre-commit` hooks. You could find more information about `pre-commit` [here](https://pre-commit.com/).

<div align="right"><a href="#table-of-contents"><img src="img/top.png" width="32"></div>

![-----------------------------------------------------](img/rainbow.png)

## Contribution

### Contribution Guide

### Contribution Workflow

### Acknowledgement

### Citations

<div align="right"><a href="#table-of-contents"><img src="img/top.png" width="32"></div>

![-----------------------------------------------------](img/rainbow.png)

## Issues?

This repository is maintained actively, so if you face any issue please <a href="https://github.com/zoeyzyhu/pydisort/issues">raise an issue</a>.

Not sure where to start? Join our discord and we will help you get started!

<a href="https://discord.gg/ZKBZg5K2"><img src="img/discord.png" width="150"/></a> &nbsp;&nbsp; <a target="_blank" href="https://bmc.link/zoeyzyhu"><img src="img/bmc.png" alt="Buy me a coffee" width="150"/></a>

---

## Extra Information

There are three formal components in this repo:

- `cdisort_patches`
- `cppdisort`
- `pydisort`

The `cdisort_patches` contains the original `cdisort` library (v2.1.3) by Timothy E. Dowling, plus some patch files containing modifications made by [Cheng Li](https://chengcli.io/). Different from `cdisort`, which uses `Makefile` to build the library, we modified the configuration and adapted a `CMake`-built approach. For detailed changes, please see the `README.md` file in the `cdisort_patches` .

The `cppdisort` provides a C++ wrapper for the "cdisort" library, allowing easy access to its functionality from C++ code. We use toml++ for configuration management, allowing users to specify various parameters in the TOML configuration file. The updated implementation ensures compatibility with modern C++ standards and incorporates bug fixes and enhancements compared to the original cdisort library. For detailed changes, please see the `README.md` file in the `cppdisort` .

The `pydisort` builds a Python library that provides a Pythonic interface to the cppdisort library. It serves as a bridge between the C++ implementation of cppdisort and the Python programming language, enabling users to leverage the power of cppdisort within their Python applications. For detailed changes, please see the `README.md` file in the `pydisort` .

#### Files

The "cppdisort.h" file includes the necessary headers and defines the DisortWrapper class. This class provides a C++ interface to interact with the "cdisort" library. It includes member variables and functions that correspond to the parameters and functions of the "cdisort" library. Some of the important member variables include `btemp`, `ttemp`, `fluor`, `albedo`, `fisot`, `fbeam`, `temis`, `umu0`, and `phi0`, which represent accessible boundary conditions. The class also provides functions for setting various parameters, such as atmosphere dimensions, flags, intensity dimensions, optical depth, single scattering albedo, level temperature, wavenumber range, output optical depth, and outgoing ray. The class also includes functions for running the radiative transfer calculations and retrieving the results.

The "cppdisort.cc" file implements the member functions of the DisortWrapper class. The functions in this file handle the initialization, parameter setting, and execution of the "cdisort" library functions. It also includes a function fromTomlTable that converts a TOML table into a DisortWrapper object.

#### Build

```bash
mkdir build
cd build
cmake ..
make
```

#### Usage

To use the wrapper, include the cppdisort.h header file in your C++ code. You can then create an instance of the DisortWrapper class and set the desired parameters using the provided methods. Finally, call the RunRTFlux method to run the radiative transfer calculations and obtain the calculated flux values.

Here's a basic example:

```c++
#include "cppdisort.h"

int main() {
    // Create an instance of DisortWrapper
    DisortWrapper disort;

    // Set atmosphere and intensity dimensions
    disort.SetAtmosphereDimension(10, 4, 16, 2)
          .SetIntensityDimension(8, 10, 20);

    // Set other parameters
    disort.SetAccuracy(1e-5)
          .SetOpticalDepth(tau, 10)
          .SetSingleScatteringAlbedo(ssa, 10)
          .SetLevelTemperature(temp, 11)
          .SetWavenumberRange_invcm(1000.0, 2000.0)
          .SetOutputOpticalDepth(usrtau, 10)
          .SetOutgoingRay(umu, phi);

    // Run radiative transfer calculations and get flux values
    auto [flxup, flxdn] = disort.RunRTFlux();

    // Process the results
    // ...

    return 0;
}
```
