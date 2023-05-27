cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(pydisort LANGUAGES CXX C)

message(STATUS "")
message(STATUS "== Setting up pydisort library ==")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message(STATUS "Default Build Type = Release")
  message(STATUS "")
else()
  message(STATUS "Build Type = ${CMAKE_BUILD_TYPE}")
  message(STATUS "")
endif()

# 1. Set up compiler flags ##
message(STATUS "1. Set up project compiler flags")
message(STATUS "Include ${CMAKE_SOURCE_DIR}/cmake/compilers.cmake")
include(${CMAKE_SOURCE_DIR}/cmake/compilers.cmake)

# 1. Set up project specific configuration ##
message(STATUS "2. Set up project specific configuration")
message(STATUS "----- Placeholder ----- ")

# 1. Set up project system libraries ##
message(STATUS "3. Set up project system libraries----------------")

message(STATUS "Setup tomlpp ")
include(${CMAKE_SOURCE_DIR}/cmake/tomlpp.cmake)

message(STATUS "Setup pybind11")
include(${CMAKE_SOURCE_DIR}/cmake/pybind11.cmake)

# 1. Set up project configure file and library ##
message(STATUS "4. Set up project libraries")
configure_file(${CMAKE_SOURCE_DIR}/configure.hpp.in configure.hpp @ONLY)
add_subdirectory(src)

# 1. Python binding
message(STATUS "5. Set up python binding")
add_subdirectory(python)

# 1. Set up unit tests
message(STATUS "6. Set up unit tests")
add_subdirectory(tests)
