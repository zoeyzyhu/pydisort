cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CUDA "Enable CUDA support" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_TESTS "Build tests" ON)

if (CUDA)
  project(
    pydisort
    LANGUAGES CXX C CUDA
    DESCRIPTION "Python-CUDA extension for disort library")
else()
  project(
    pydisort
    LANGUAGES CXX C
    DESCRIPTION "Python extension for disort library")
endif()

# search for <Package>_ROOT
cmake_policy(SET CMP0074 NEW)

# Set the _GLIBCXX_USE_CXX11_ABI flag to 0 (use old ABI)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

# Set MacOS deployment target
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")

# make colorful output
string(ASCII 27 Esc)
set(ColorReset "${Esc}[m")
set(Green      "${Esc}[32m")
set(Blue       "${Esc}[34m")

message(STATUS "")
message(STATUS "${Green}==== ${PROJECT_NAME} configure begin ====${ColorReset}")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message(STATUS "Default Build Type = Release")
  message(STATUS "")
else()
  message(STATUS "Build Type = ${CMAKE_BUILD_TYPE}")
  message(STATUS "")
endif()

# load all modules
LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/)

# load all macros
file(GLOB _macro_files "${CMAKE_SOURCE_DIR}/cmake/macros/*.cmake")
foreach(_file ${_macro_files})
  message(STATUS "Include ${_file}")
  include(${_file})
endforeach()

# set output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

message(STATUS "${Blue}${PROJECT_NAME}-1. Set up project compiler flags ...${ColorReset}")
message(STATUS "Include ${CMAKE_CURRENT_SOURCE_DIR}/cmake/compilers.cmake")
include(${CMAKE_SOURCE_DIR}/cmake/compilers.cmake)

message(STATUS "${Blue}${PROJECT_NAME}-2. Setting up project parameters ...${ColorReset}")

message(STATUS "${Blue}${PROJECT_NAME}-3. Setting up system libraries ...${ColorReset}")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/fmt.cmake)
find_program(Python3_EXECUTABLE NAMES python)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(Torch REQUIRED)
if (CUDA)
  find_package(CUDAToolkit REQUIRED)
else()
  set(CUDAToolkit_FOUND OFF)
endif()

if (CUDAToolkit_FOUND)
  set(CMAKE_CUDA_ARCHITECTURES 61 86 89)
  set(CMAKE_CUDA_STANDARD 17)
endif()

message(STATUS "${Blue}${PROJECT_NAME}-4. Setting up project libraries ...${ColorReset}")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/python/VERSION" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
               ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h @ONLY)
add_subdirectory(src)
add_subdirectory(cdisort213)

if (BUILD_TESTS)
  message(STATUS "${Blue}${PROJECT_NAME}-5. Setting up unit tests ...${ColorReset}")
  add_subdirectory(tests)
endif()

if (BUILD_EXAMPLES)
  message(STATUS "${Blue}${PROJECT_NAME}-6. Setting up examples ...${ColorReset}")
  add_subdirectory(examples)
endif()
