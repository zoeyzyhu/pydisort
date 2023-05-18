cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(cppdisort LANGUAGES CXX C)

message(STATUS "")
message(STATUS "== Setting up cppdisort library ==")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message(STATUS "Default Build Type = Release")
  message(STATUS "")
else()
  message(STATUS "Build Type = ${CMAKE_BUILD_TYPE}")
  message(STATUS "")
endif()


## 1. Set up compiler flags ##
message(STATUS "1. Set up project compiler flags")
message(STATUS "Include ${CMAKE_SOURCE_DIR}/cmake/setup_compiler_flags.cmake")
include(${CMAKE_SOURCE_DIR}/cmake/setup_compiler_flags.cmake)


## 2. Set up project specific configuration ##
message(STATUS "2. Set up project specific configuration")
message(STATUS "----- Placeholder ----- ")


## 3. Set up project system libraries ##

message(STATUS "3.1. Set up project system libraries----------------")
message(STATUS "Setup tomlpp ")
include(${CMAKE_SOURCE_DIR}/cmake/tomlpp.cmake)

message(STATUS "3.2. Set the path to Python headers ----------------")
# 3.2.1 Execute python3-config to get the include flags
execute_process(
    COMMAND python3-config --includes
    OUTPUT_VARIABLE PYTHON_INCLUDE_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
# 3.2.2 Extract the include path from the flags
string(REGEX MATCH "-I([^\\\"]+)" PYTHON_INCLUDE_PATH "${PYTHON_INCLUDE_FLAGS}")
# 3.2.3 Add the Python include path to include_directories
include_directories(${PYTHON_INCLUDE_PATH})
include(${CMAKE_SOURCE_DIR}/cmake/pybind11.cmake)



## 4. Set up project configure file and library ##
message(STATUS "5. Set up project libraries")
configure_file(${CMAKE_SOURCE_DIR}/configure.hpp.in configure.hpp @ONLY)
add_subdirectory(src)

## 5. Python binding
message(STATUS "5. Set up python binding")
add_subdirectory(python)

## 6. Set up unit tests
message(STATUS "6. Set up unit tests")
add_subdirectory(tests)


