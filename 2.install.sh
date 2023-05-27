#!/bin/bash

# ====================================================
# `source env/bin/activate`` before running this
# ====================================================

# Upgrade pip and setuptools
pip3 install --upgrade pip setuptools

# Install necessary packages
pip3 install -r requirements.txt

# Show the packages installed
pip3 list

# Install the git hook scripts
pre-commit install
