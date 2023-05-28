<h4 align="center">
    <img src="img/logo.png" alt="Pydisort" width="300">
</h4>

<p align="center">
  <i align="center">Empower the usage of Discrete Ordinates Radiative Transfer (DISORT) with Python 🚀</i>
</p>

<p align="center">
<a target="_blank" href="https://github.com/zoeyzyhu/actions/workflows/main.yml"><img src="https://github.com/zoeyzyhu/pydisort/actions/workflows/main.yml/badge.svg"/></a>
<a target="_blank" href="https://www.codacy.com/gh/ankitwasankar/mftool-java/dashboard?utm_source=github.com&utm_medium=referral&utm_content=ankitwasankar/mftool-java&utm_campaign=Badge_Coverage"><img src="https://app.codacy.com/project/badge/Coverage/0054db87ea0f426599c3a30b39291388" /></a>
<a href="https://www.codacy.com/gh/ankitwasankar/mftool-java/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=ankitwasankar/mftool-java&amp;utm_campaign=Badge_Grade"><img src="https://app.codacy.com/project/badge/Grade/0054db87ea0f426599c3a30b39291388"/></a>
<a target="_blank" href="https://github.com/ankitwasankar/mftool-java/blob/master/license.md"><img src="https://camo.githubusercontent.com/8298ac0a88a52618cd97ba4cba6f34f63dd224a22031f283b0fec41a892c82cf/68747470733a2f2f696d672e736869656c64732e696f2f707970692f6c2f73656c656e69756d2d776972652e737667" /></a>
&nbsp <a target="_blank" href="https://www.linkedin.com/in/ankitwasankar/"><img height="20" src="https://img.shields.io/badge/LinkedIn-0077B5?style=for-the-badge&logo=linkedin&logoColor=white" /></a>
</p>
<p align="center">
  This repository contains a Python package and a C++ wrapper for the <code>cdisort</code> library, which is a C implementation of the DISORT radiative transfer model.
</p>

<p align="center">
<a href="#introduction">Introduction</a> &nbsp;&bull;&nbsp;
<a href="#usage">Usage</a> &nbsp;&bull;&nbsp;
<a href="#contribution">Contribution</a> &nbsp;&bull;&nbsp;
<a href="#issues">Issues?</a> &nbsp;&bull;&nbsp;
</p>

# Introduction

## Placeholder

There are three formal components in this repo:

- `cdisort_patches`
- `cppdisort`
- `pydisort`

The `cdisort_patches` contains the original `cdisort` library (v2.1.3) by Timothy E. Dowling, plus some patch files containing modifications made by [Cheng Li](https://chengcli.io/). Different from `cdisort`, which uses `Makefile` to build the library, we modified the configuration and adapted a `CMake`-built approach. For detailed changes, please see the `README.md` file in the `cdisort_patches` .

The `cppdisort` provides a C++ wrapper for the "cdisort" library, allowing easy access to its functionality from C++ code. We use toml++ for configuration management, allowing users to specify various parameters in the TOML configuration file. The updated implementation ensures compatibility with modern C++ standards and incorporates bug fixes and enhancements compared to the original cdisort library. For detailed changes, please see the `README.md` file in the `cppdisort` .

The `pydisort` builds a Python library that provides a Pythonic interface to the cppdisort library. It serves as a bridge between the C++ implementation of cppdisort and the Python programming language, enabling users to leverage the power of cppdisort within their Python applications. For detailed changes, please see the `README.md` file in the `pydisort` .

# Usage

## For Python users

## For developers: The C++ Wrapper for `cdisort`

This branch provides a wrapper for the "cdisort" library which is implemented in C++. The wrapper allows accessing the functionality of the "cdisort" library from C++ code. The wrapper consists of two files: "cppdisort.h" and "cppdisort.cc".

### Files

The "cppdisort.h" file includes the necessary headers and defines the DisortWrapper class. This class provides a C++ interface to interact with the "cdisort" library. It includes member variables and functions that correspond to the parameters and functions of the "cdisort" library. Some of the important member variables include `btemp`, `ttemp`, `fluor`, `albedo`, `fisot`, `fbeam`, `temis`, `umu0`, and `phi0`, which represent accessible boundary conditions. The class also provides functions for setting various parameters, such as atmosphere dimensions, flags, intensity dimensions, optical depth, single scattering albedo, level temperature, wavenumber range, output optical depth, and outgoing ray. The class also includes functions for running the radiative transfer calculations and retrieving the results.

The "cppdisort.cc" file implements the member functions of the DisortWrapper class. The functions in this file handle the initialization, parameter setting, and execution of the "cdisort" library functions. It also includes a function fromTomlTable that converts a TOML table into a DisortWrapper object.

### Build

```bash
mkdir build
cd build
cmake ..
make
```

### Usage

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

# Issues?

This repository is maintained actively, so if you face any issue please <a href="https://github.com/zoeyzyhu/pydisort/issues">raise an issue</a>.

Not sure where to start? Join our discord and we will help you get started!

<a href="https://discord.gg/ZKBZg5K2"><img src="img/discord.png" width="200"/></a>
