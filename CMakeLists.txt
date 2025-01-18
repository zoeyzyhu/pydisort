cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(
  pydisort
  LANGUAGES CXX C CUDA
  DESCRIPTION "Python-CUDA extension for disort library")

# search for <Package>_ROOT
cmake_policy(SET CMP0074 NEW)

# Set the _GLIBCXX_USE_CXX11_ABI flag to 0 (use old ABI)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

set(CMAKE_CUDA_ARCHITECTURES 61 86 89)
set(CMAKE_CUDA_STANDARD 17)
set(BUILD_SHARED_LIBS OFF)

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

option(PYTHON_BINDINGS "Build Python Bindings" ON)

message(STATUS "${Blue}${PROJECT_NAME}-1. Set up project compiler flags ...${ColorReset}")
message(STATUS "Include ${CMAKE_CURRENT_SOURCE_DIR}/cmake/compilers.cmake")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compilers.cmake)

message(STATUS "${Blue}${PROJECT_NAME}-2. Setting up system libraries ...${ColorReset}")
if(PYTHON_BINDINGS)
  set(Python_FIND_VIRTUALENV FIRST)
  find_package(Python3 ${PYTHON_VERSION} REQUIRED
               COMPONENTS Interpreter Development.Module)
endif()
find_package(Torch REQUIRED)
include(${CMAKE_SOURCE_DIR}/cmake/pybind11.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/fmt.cmake)

message(STATUS "${Blue}${PROJECT_NAME}-3. Setting up project libraries ...${ColorReset}")
add_subdirectory(src)
add_subdirectory(cdisort213)

if(PYTHON_BINDINGS)
  add_subdirectory(python)
endif()

message(STATUS "${Blue}${PROJECT_NAME}-4. Setting up unit tests ...${ColorReset}")
#add_subdirectory(tests)

#message(STATUS "${Blue}${PROJECT_NAME}-6. Setting up examples ...${ColorReset}")
#add_subdirectory(examples)
