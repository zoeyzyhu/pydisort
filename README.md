# 1. Overview of the Project

## DISORT in Fortran, C, C++, Python

You can find the detailed overview of the DISORT project, including information about disort, cdisort, cppdisort, and pydisort @ [README - Overview of the DISORT project in Fortran, C, C++, Python](./README_DISORT.md).

## Major Branches

There are three formal branches in this repo:
- `cdisort_patches`
- `cppdisort`
- `pydisort`

The `cdisort_patches` branch contains the original `cdisort` library (v2.1.3) by Timothy E. Dowling, plus some patch files containing modifications made by [Cheng Li](https://chengcli.io/). Different from `cdisort`, which uses `Makefile` to build the library, we modified the configuration and adapted a `CMake`-built approach. For detailed changes, please see the `README.md` file in the `cdisort_patches` branch.

The `cppdisort` branch provides a C++ wrapper for the "cdisort" library, allowing easy access to its functionality from C++ code. We use toml++ for configuration management, allowing users to specify various parameters in the TOML configuration file. The updated implementation ensures compatibility with modern C++ standards and incorporates bug fixes and enhancements compared to the original cdisort library. For detailed changes, please see the `README.md` file in the `cppdisort` branch.

The `pydisort` branch builds a Python library that provides a Pythonic interface to the cppdisort library. It serves as a bridge between the C++ implementation of cppdisort and the Python programming language, enabling users to leverage the power of cppdisort within their Python applications. For detailed changes, please see the `README.md` file in the `pydisort` branch.


# 2. This Branch: a C++ Wrapper for `cdisort`

This branch provides a wrapper for the "cdisort" library which is implemented in C++. The wrapper allows accessing the functionality of the "cdisort" library from C++ code. The wrapper consists of two files: "cppdisort.h" and "cppdisort.cc".

## Files

The "cppdisort.h" file includes the necessary headers and defines the DisortWrapper class. This class provides a C++ interface to interact with the "cdisort" library. It includes member variables and functions that correspond to the parameters and functions of the "cdisort" library. Some of the important member variables include `btemp`, `ttemp`, `fluor`, `albedo`, `fisot`, `fbeam`, `temis`, `umu0`, and `phi0`, which represent accessible boundary conditions. The class also provides functions for setting various parameters, such as atmosphere dimensions, flags, intensity dimensions, optical depth, single scattering albedo, level temperature, wavenumber range, output optical depth, and outgoing ray. The class also includes functions for running the radiative transfer calculations and retrieving the results.

The "cppdisort.cc" file implements the member functions of the DisortWrapper class. The functions in this file handle the initialization, parameter setting, and execution of the "cdisort" library functions. It also includes a function fromTomlTable that converts a TOML table into a DisortWrapper object.

## Build

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

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

## Summary

Overall, the `cppdisort` wraps the "cdisort" library in C++ using the DisortWrapper class, allowing easier access and usage of the library's functionality from C++ code.  It provides an object-oriented, encapsulated, and more user-friendly interface to the functionality of "cdisort". It leverages the benefits of C++ to offer improved usability, convenience, and potentially enhanced performance compared to using "cdisort" directly.
