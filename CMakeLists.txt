cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(pydisort 
  LANGUAGES CXX C
  DESCRIPTION "Python bindings for disort library"
  )

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message(STATUS "Default Build Type = Release")
  message(STATUS "")
else()
  message(STATUS "Build Type = ${CMAKE_BUILD_TYPE}")
  message(STATUS "")
endif()

option(PYTHON_BINDINGS "Build Python Bindings" ON)

## 1. Set up compiler flags ##
message(STATUS "1. Set up project compiler flags ...")
message(STATUS "Include ${CMAKE_CURRENT_SOURCE_DIR}/cmake/compilers.cmake")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compilers.cmake)

## 2. Set up project system libraries ##
message(STATUS "2. Set up project system libraries ...")
include(${CMAKE_SOURCE_DIR}/cmake/pybind11.cmake)

# 1. Set up project configure file and library ##
message(STATUS "3. Set up project libraries ...")
add_subdirectory(src)
add_subdirectory(interface)

set(PYDISORT_INCLUDE_DIR 
  "${CMAKE_CURRENT_SOURCE_DIR}"
  CACHE PATH "Path to minichem include directory")

if(PYTHON_BINDINGS)
  add_subdirectory(python)
endif()

# 1. Set up unit tests
message(STATUS "4. Set up unit tests ...")
add_subdirectory(tests)
