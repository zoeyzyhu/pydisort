#! /bin/bash

filters=-legal/copyright,-build/include_subdir,-build/include_order,-runtime/printf

# cppdisort
cpplint --filter=${filters} --recursive src/cppdisort

# pybind_cppdisort
cpplint --filter=${filters} --recursive python/*.cpp
