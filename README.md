<h4 align="center">
    <img src="img/logo.png" alt="Pydisort" width="300" style="display: block; margin: 0 auto">
</h4>

<p align="center">
  <i align="center">Empower Discrete Ordinates Radiative Transfer (DISORT) with Python 🚀</i>
</p>

<p align="center">
<a target="_blank" href="https://github.com/zoeyzyhu/actions/workflows/main.yml"><img alt="GitHub Workflow Status" src="https://img.shields.io/github/actions/workflow/status/zoeyzyhu/pydisort/main.yml?style=flat-square&logo=github"></a>
<a target="_blank" href="https://www.codacy.com/gh/ankitwasankar/mftool-java/dashboard?utm_source=github.com&utm_medium=referral&utm_content=ankitwasankar/mftool-java&utm_campaign=Badge_Coverage"><img src="https://app.codacy.com/project/badge/Coverage/0054db87ea0f426599c3a30b39291388?style=flat-square" /></a>
<a target="_blank" href="https://codecov.io/gh/{{REPOSITORY}}"><img src="https://codecov.io/gh/{{REPOSITORY}}/branch/main/graph/badge.svg?style=flat-square?style=flat-square" /></a>
<a href="https://github.com/zoeyzyhu/pydisort/issues">
<img alt="GitHub issues" src="https://img.shields.io/github/issues/zoeyzyhu/pydisort?style=flat-square&logo=git""></a>
<a target="_blank" href="https://github.com/zoeyzyhu/pydisort/releases"><img alt="GitHub release (latest by date)" src="https://img.shields.io/github/v/release/zoeyzyhu/pydisort?style=flat-square&logo=buffer"></a>
<br>
<a href="https://github.com/pre-commit/pre-commit"><img src="https://img.shields.io/badge/pre--commit-enabled-brightgreen?style=flat-square&logo=pre-commit" alt="pre-commit" style="max-width:100%;"></a>
<a href="http://makeapullrequest.com"><img src="https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square&logo=pre-commit" alt="pre-commit" style="max-width:100%;"></a>
<a target="_blank" href="https://opensource.org/licenses/"><img src="https://img.shields.io/badge/License-GPL-yellow.svg?style=flat-square&logo=gnu" /></a>
<a target="_blank" href="https://img.shields.io/badge/OS-Linux%2C%20MacOS-orange"><img src="https://img.shields.io/badge/OS-linux%2C%20mac-orange?style=flat-square&logo=linux" /></a>

</p>
<p align="center">
  This repository contains a Python package and a C++ wrapper for the <code>cdisort</code> library, which is a C implementation of the DISORT radiative transfer model.
</p>

<p align="center">
<a href="#introduction">Introduction</a> &nbsp;&bull;&nbsp;
<a href="#usage">Usage</a> &nbsp;&bull;&nbsp;
<a href="#contribution">Contribution</a> &nbsp;&bull;&nbsp;
<a href="#issues">Issues?</a>
</p>

---

## Introduction

### Placeholder

There are three formal components in this repo:

- `cdisort_patches`
- `cppdisort`
- `pydisort`

The `cdisort_patches` contains the original `cdisort` library (v2.1.3) by Timothy E. Dowling, plus some patch files containing modifications made by [Cheng Li](https://chengcli.io/). Different from `cdisort`, which uses `Makefile` to build the library, we modified the configuration and adapted a `CMake`-built approach. For detailed changes, please see the `README.md` file in the `cdisort_patches` .

The `cppdisort` provides a C++ wrapper for the "cdisort" library, allowing easy access to its functionality from C++ code. We use toml++ for configuration management, allowing users to specify various parameters in the TOML configuration file. The updated implementation ensures compatibility with modern C++ standards and incorporates bug fixes and enhancements compared to the original cdisort library. For detailed changes, please see the `README.md` file in the `cppdisort` .

The `pydisort` builds a Python library that provides a Pythonic interface to the cppdisort library. It serves as a bridge between the C++ implementation of cppdisort and the Python programming language, enabling users to leverage the power of cppdisort within their Python applications. For detailed changes, please see the `README.md` file in the `pydisort` .

---

## Table of Contents

- [Introduction](#introduction)
- [Usage](#usage)

<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#introduction"> Introduction</a></li>
    <li><a href="#usage"> Usage</a></li>
    <li><a href="#contribution"> Contribution</a></li>
    <li><a href="#issues"> Issues?</a></li>
  </ol>
</details>

---

## Usage

### For Python users

### For developers: The C++ Wrapper for `cdisort`

This branch provides a wrapper for the "cdisort" library which is implemented in C++. The wrapper allows accessing the functionality of the "cdisort" library from C++ code. The wrapper consists of two files: "cppdisort.h" and "cppdisort.cc".

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

<div align="right"><a href="#table-of-contents"><img align="right" src="img/top.png" width="34" ></a></div>

---

# Contribution

<div align="right"><a href="#table-of-contents"><img align="right" src="img/top.png" width="34" ></a></div>

---

## Issues?

This repository is maintained actively, so if you face any issue please <a href="https://github.com/zoeyzyhu/pydisort/issues">raise an issue</a>.

Not sure where to start? Join our discord and we will help you get started!

<a href="https://discord.gg/ZKBZg5K2"><img src="img/discord.png" width="150"/></a>

<a target="_blank" href="https://bmc.link/zoeyzyhu"><img src="img/bmc.png" alt="Buy me a coffee" width="150"/></a>
