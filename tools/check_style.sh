#! /bin/bash

# Style checks for C++ ====================================================

filters=-legal/copyright,-build/include_subdir,-build/include_order,-runtime/references

# cppdisort
cpplint --filter=${filters} --recursive src/cppdisort

# pybind_cppdisort
cpplint --filter=${filters} --recursive python/*.cpp

# test_cppdisort
cpplint --filter=${filters} --recursive tests/cppdisort

# Style checks for Python =================================================

pylint --recursive=y tests/python