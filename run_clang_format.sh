#!/bin/bash

# Directories where the source files are present
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

