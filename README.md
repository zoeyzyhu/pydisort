<!-- Logo ------------------------------------------->
<h4 align="center">
    <img src="doc/img/logo.png" alt="Pydisort" width="300" style="display: block; margin: 0 auto">
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
<a href="https://github.com/zoeyzyhu/actions/workflows/main.yml">
  <img alt="GitHub Workflow Status"
    src="https://img.shields.io/github/actions/workflow/status/zoeyzyhu/pydisort/main.yml?style=flat-square&logo=github"/>
</a>
<a href="https://codecov.io/gh/{{REPOSITORY}}">
  <img alt="Codacy coverage"
    src="https://img.shields.io/codacy/coverage/pydisort?style=flat-square&logo=codecov"/>
</a>
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
  <img alt="pre-commit"
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

![-----------------------------------------------------](doc/img/rainbow.png)

## Table of Contents

- [Introduction](#introduction)
- [How to use](#usage)
  - [For Python users](#for-python-users)
  - [For C++ developers](#for-c++-users)
    - [Check dependencies](#check-dependencies)
    - [Build and run the C++ wrapper](#build-and-run-the-c++-wrapper)
    - [Build and run the Python package](#build-and-run-the-python-package)
- [Contributing](#contributing)
- [Issues?](#issues)

![-----------------------------------------------------](doc/img/rainbow.png)

## How to use

<!-- For Python users-------------------------------->

### <a id='for-python-users'><picture><img src="doc/img/python.svg" alt="Python" align=left width=24></picture> For Python users</a>

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

Please note that this is a generic tutorial and you would need to adapt this to your specific use-case.

> 💡 We keep the parameters consistent to the original `DISORT` library, so you can refer to the [DISORT documentation](src/cdisort/DISORT2.doc) for more information such as input/out variables, flags, model usage and caveats.

For example, you might need to provide your own data file in `from_file` function or fill the numpy arrays `optical_depth`, `single_scattering_albedo`, and `level_temperature` according to your requirements.

> 💡 One important point to note is that the `pydisort` library assumes that the provided arrays (optical depth, single scattering albedo, etc.) are in the numpy format and it throws exceptions if incompatible data types are provided. So, ensure that you are providing data in the right format to avoid any runtime errors.

<div align="right"><a href="#table-of-contents"><img src="doc/img/top.png" width="32"></div>

[//]: <> (!!Do not remove the following line, which is used for dividing the content)

#

<!-- For C++ developers------------------------------>

### <a id='for-c++-users'><picture><img src="doc/img/cpp.svg" alt="C++" align=left width=24></picture> For C++ developers</a>

#### <a id='check-dependencies'> 🔻 Check dependences</a>

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

#### <a id='build-and-run-the-c++-wrapper'> 🔻 Build and run the C++ wrapper</a>

You could fork or clone this repository to your local machine.

```bash
git clone https://github.com/zoeyzyhu/pydisort.git
```

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
cd bin

# Run the C++ wrapper
./test_cppdisort.release

# If you're interested in `cdisort`, run the test provided by Dowling
./test_cdisort.release
```

#### <a id='build-and-run-the-python-package'> 🔻 Build and run the Python package</a>

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

<div align="right"><a href="#table-of-contents"><img src="doc/img/top.png" width="32"></div>

![-----------------------------------------------------](doc/img/rainbow.png)

## Contributing

[![Good first issues open](https://img.shields.io/github/issues/zoeyzyhu/pydisort/good%20first%20issue?label=good%20first%20issues&logo=git&logoColor=white&style=flat-square)](https://github.com/zoeyzyhu/pydisort/labels/good%20first%20issue)

Pull-Requests are welcomed. Fork repository, make changes, send us a pull request. We will review your changes and apply them to the main branch shortly, provided they don't violate our quality standards. Please read the [contribution guide](doc/CONTRIBUTING.md) for details on the workflow, conventions, etc.

If you need to make changes to the `cdisort` library, please use patches to record your modification. We keep a sole branch called `cidosrt_patches`, which contains the cmake-built version of the `cdisort` library (v2.1.3) and all the patches that we have applied to it. Please refer to the [patching guide](doc/README-patches.md) for more information.

If you need to include more libraries to the `cppdisrot` wrapper, please use the `CMakeLists.txt` file to add them. You could find more information about the cmake build system [here](https://cmake.org/cmake/help/latest/guide/tutorial/index.html).

If you need to make changes to the `pydisort` package, please use the `pybind11` library to bind the C++ wrapper to Python, expose the functions and classes to Python, and add more test cases to the `pydisort` package. You could find more information about the `pybind11` library [here](https://pybind11.readthedocs.io/en/stable/).

For more information to assist your development, please refer to the `doc/` folder in this repository.

<div align="right"><a href="#table-of-contents"><img src="doc/img/top.png" width="32"></div>

![-----------------------------------------------------](doc/img/rainbow.png)

## Issues?

This repository is maintained actively, so if you face any issue please <a href="https://github.com/zoeyzyhu/pydisort/issues">raise an issue</a>.

Not sure where to start? Join our discord and we will help you get started!

<a href="https://discord.gg/ZKBZg5K2"><img src="doc/img/discord.png" width="150"/></a> &nbsp;&nbsp; <a target="_blank" href="https://bmc.link/zoeyzyhu"><img src="doc/img/bmc.png" alt="Buy me a coffee" width="150"/></a>
