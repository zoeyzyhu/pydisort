# An Overview of the DISORT Project

## Fortran-DISORT

DISORT (Discrete Ordinate Radiative Transfer) is a widely-used radiative transfer algorithm that computes the scattering and absorption of radiation in a medium. It is commonly employed in atmospheric and remote sensing studies to model the transfer of solar and thermal radiation through the Earth's atmosphere. The DISORT algorithm provides accurate and efficient calculations of radiative transfer in complex atmospheric conditions.

DISORT solves the radiative transfer equation by discretizing the atmosphere into a set of layers and solving the equations for each layer independently. It considers multiple scattering effects and can handle various types of scattering particles, including molecules, aerosols, and clouds. DISORT is known for its versatility, enabling simulations for a wide range of atmospheric conditions and geometries. The DISORT library is originally written in Fortran.

## CDISORT

CDISORT is a C library that provides an interface to the DISORT algorithm. It offers a set of functions and data structures for configuring the parameters of the radiative transfer calculation and running the DISORT calculations. CDISORT simplifies the usage of DISORT by providing a higher-level interface that can be accessed from C code.

The CDISORT library encapsulates the complexity of the DISORT algorithm and provides a convenient way to perform radiative transfer calculations without having to deal with low-level implementation details. It abstracts away the underlying algorithms and provides a straightforward API for setting up the atmospheric conditions, specifying input parameters, and obtaining the resulting flux values.

The library is typically used by researchers and scientists working in fields such as atmospheric physics, climate modeling, remote sensing, and radiative transfer simulations. It facilitates the integration of the DISORT algorithm into existing software or the development of new applications that require radiative transfer calculations.

## CPPDISORT

CPPDISORT is a C++ library that builds upon the CDISORT library and provides an object-oriented interface for utilizing the DISORT radiative transfer algorithm. It aims to simplify the usage of DISORT in C++ applications by providing a modern and intuitive interface.

CPPDISORT leverages the power of C++ to encapsulate the functionality of the DISORT algorithm into classes and objects. This object-oriented approach makes it easier to manage the radiative transfer calculations, configure input parameters, and retrieve the results.

By using CPPDISORT, developers can write cleaner and more maintainable code when working with the DISORT algorithm. The library abstracts away low-level implementation details and provides a higher-level interface that is consistent with modern C++ programming practices. It takes advantage of features such as classes, inheritance, and polymorphism to provide a flexible and extensible framework for radiative transfer simulations.

CPPDISORT inherits all the capabilities of the underlying DISORT algorithm, allowing users to accurately model the scattering and absorption of radiation in complex atmospheric conditions. It provides a seamless integration with existing C++ projects and can be easily extended to incorporate additional functionality or customize the radiative transfer calculations according to specific requirements.

CPPDISORT is designed to cater to the needs of researchers, scientists, and developers working in fields such as atmospheric physics, climate modeling, remote sensing, and related disciplines. It offers a modern and efficient solution for performing radiative transfer simulations in C++ applications, enabling the exploration and analysis of complex atmospheric phenomena.

## PYDISORT

The primary goal of PYDISORT is to allow Python developers to easily access and utilize the functionality provided by cppdisort without having to write C++ code. By using PYDISORT, users can take advantage of the efficient radiative transfer calculations of cppdisort while enjoying the flexibility and ease of use of the Python language.

PYDISORT leverages the pybind11 library, which is a lightweight header-only library that exposes C++ types in Python and vice versa. It facilitates the seamless integration of cppdisort with Python by generating the necessary bindings and allowing C++ functions and classes to be called from Python code.

With PYDISORT, users can perform radiative transfer simulations using the DISORT algorithm directly from their Python applications. They can configure input parameters, invoke the radiative transfer calculations, and retrieve the results in a Pythonic way, simplifying the overall workflow.

By providing a Python interface to cppdisort, PYDISORT opens up a wide range of possibilities for scientific analysis, data visualization, and integration with other Python libraries and tools. It enables researchers, scientists, and developers in fields such as atmospheric physics, climate modeling, and remote sensing to leverage the capabilities of cppdisort in their Python-based workflows.

PYDISORT strives to offer a user-friendly and intuitive API that aligns with Python programming conventions, making it easier for users to understand and work with the library. It aims to be a valuable resource for the Python community by providing an efficient and accessible solution for radiative transfer simulations in atmospheric science and related domains.

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
