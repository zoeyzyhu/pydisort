#!/bin/bash

# ====================================================
# `source env/bin/activate`` before running this
# ====================================================

# Upgrade pip and setuptools
pip install --upgrade pip setuptools

# Install necessary packages
pip install -r requirements.txt

# Show the packages installed
pip list

# Install the git hook scripts
pre-commit install
