#!/bin/bash

# Stop on errors, print commands
set -xEeuo pipefail

# Test in C++
cd ./build/bin
./test_cppdisort.release

# Test in Python3
python3 test_isotropic_scattering.py

# Back to root dir
cd ../..