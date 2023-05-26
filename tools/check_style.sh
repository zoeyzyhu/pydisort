#! /bin/bash

filters=-legal/copyright,-build/include_subdir,runtime/references

# cppdisort
cpplint --filter=${filters} --recursive src/cppdisort

# pybind_cppdisort
cpplint --filter=${filters} --recursive python/*.cpp
