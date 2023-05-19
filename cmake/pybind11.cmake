include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

# Execute python3-config to get the include flags
execute_process(
    COMMAND python3-config --includes
    OUTPUT_VARIABLE PYTHON_INCLUDE_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
# 3.2.2 Extract the include path from the flags
#string(REGEX MATCH "-I([^\\\"]+)" PYTHON_INCLUDE_PATH "${PYTHON_INCLUDE_FLAGS}")
# 3.2.3 Add the Python include path to include_directories
#include_directories(${PYTHON_INCLUDE_PATH})

FetchContent_Declare(
    pybind11
    URL https://github.com/pybind/pybind11/archive/v2.10.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

FetchContent_GetProperties(pybind11)

if(NOT pybind11_POPULATED)
    FetchContent_Populate(pybind11)
    add_subdirectory(${pybind11_SOURCE_DIR} ${pybind11_BINARY_DIR})
endif()

set(PYBIND11_INCLUDE_DIR ${pybind11_SOURCE_DIR}/include
    CACHE PATH 
    "include directory of pybind11")
