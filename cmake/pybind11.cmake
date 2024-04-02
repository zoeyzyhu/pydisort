include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

if(PYTHON_BINDINGS)
  if(NOT DEFINED PYTHON_VERSION)
    find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter Development.Module)
  else()
    set(Python3_FIND_STRATEGY VERSION)
    find_package(Python3 ${PYTHON_VERSION} EXACT REQUIRED COMPONENTS
      Interpreter Development.Module)
  endif()
  set(PYTHON_BINDINGS_OPTION "PYTHON_BINDINGS")

  FetchContent_Declare(
    pybind11
    DOWNLOAD_EXTRACT_TIMESTAMP
    TRUE
    URL https://github.com/pybind/pybind11/archive/v2.11.1.tar.gz)

  FetchContent_MakeAvailable(pybind11)

  include_directories(pybind11::headers)
endif()
