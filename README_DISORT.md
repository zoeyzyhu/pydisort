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
