#! /bin/bash

# 1. Style checks for C++ ====================================================

filters=-legal/copyright,-build/include_subdir,-runtime/references

# cppdisort
cpplint --filter=${filters} --recursive src/cppdisort

# pybind_cppdisort
cpplint --filter=${filters} --recursive python/*.cpp

# test_cppdisort
cpplint --filter=${filters} --recursive tests/cppdisort

# 2. Style checks for Python =================================================

# -- Need to finish building and install pydisort to pass
# -- tests/python/test_isotropic_scattering.py:8:0:
#    E0401: Unable to import 'numpy' (import-error)
# -- tests/python/test_isotropic_scattering.py:9:0:
#    E0401: Unable to import 'numpy.testing' (import-error)
# -- tests/python/test_isotropic_scattering.py:11:0:
#    E0401: Unable to import 'pydisort' (import-error)

# pylint --recursive=y tests/python
