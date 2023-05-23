#!/bin/bash
# This is an example script to run clang-format on all source files in specified
# directories. This script is not used by the build system, but can be used to
# format all source files for other purposes.

# Specify directories where the source files are present
dir_paths=(
    "src/cppdisort"
    "python"
)

for dir_path in "${dir_paths[@]}"
do
  # Recursively find and format all .cc, .cpp, .h and .hpp files
  find $dir_path \( -iname *.cc -o -iname *.cpp -o -iname *.h -o -iname *.hpp \) | while read f
  do
    echo "Formatting $f"
    clang-format -i -style=Google $f
  done
done
