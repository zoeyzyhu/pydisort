include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

if(PYTHON_BINDINGS)
  FetchContent_Declare(
    pybind11
    DOWNLOAD_EXTRACT_TIMESTAMP
    TRUE
    URL https://github.com/pybind/pybind11/archive/v2.11.1.tar.gz)

  FetchContent_GetProperties(pybind11)

  if(NOT pybind11_POPULATED)
    FetchContent_Populate(pybind11)
    add_subdirectory(${pybind11_SOURCE_DIR} ${pybind11_BINARY_DIR})
  endif()

  set(PYBIND11_INCLUDE_DIR
      pybind11::headers
      CACHE PATH "include directory of pybind11")
endif()
