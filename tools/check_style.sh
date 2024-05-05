#! /bin/bash

# Style checks for C++ ====================================================

filters1=-legal/copyright,-build/include_subdir,-build/include_order,-runtime/references
filters2=-legal/copyright,-readability/fn_size

# cppdisort
cpplint --filter=${filters1} --recursive cppdisort

# pybind_cppdisort
cpplint --filter=${filters2} --recursive python/*.cpp

# test_cppdisort
cpplint --filter=${filters1} --recursive tests/interface/*.cpp

# Style checks for Python =================================================

pylint --recursive=y tests/python
