#!/bin/bash

# ====================================================
# This script assumes that you have Python3 installed
# ====================================================

# Stop on errors, print commands
set -xEeuo pipefail

# 1. Setup Python virtual env ------------------------

# Clear pre-existing environment settings
DIR=env
if [ -d "$DIR" ]; then
    rm -rf $DIR
fi

# Create the Python virtual environment
python3 -m venv $DIR


# 2. Setup packages for pre-commit hooks -------------
if ! which cppcheck > /dev/null; then
    if command -v apt-get >/dev/null; then
      sudo apt-get install cppcheck
    elif command -v yum >/dev/null; then
      sudo yum install cppcheck
    elif command -v brew; then
      brew install cppcheck
    else
      echo "Please install cppcheck to facilitate hooks!"
    fi
fi