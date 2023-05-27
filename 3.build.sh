#!/bin/bash

# Stop on errors, print commands
set -xEeuo pipefail

# Create and enter directory
DIR=build
if [ -d "$DIR" ]; then
    rm -rf $DIR
fi

rm -rf $DIR
mkdir $DIR
cd $DIR

# Build the library and tests
cmake ..
make
make install

# Back to root dir
cd ..