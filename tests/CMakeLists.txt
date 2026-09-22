# ============================
# A collection of unit tests
# ============================

# C++ tests
setup_test(test_disort)
setup_test(test_clone)

# Python tests
execute_process(
  COMMAND ${Python3_EXECUTABLE} -m pytest --version
  RESULT_VARIABLE PYTEST_NOT_FOUND
  OUTPUT_QUIET ERROR_QUIET)

# GLOB_RECURSE picks up tests/reference/ as well; RELATIVE keeps the paths so
# that conftest.py lands beside the modules that rely on it.
file(GLOB_RECURSE pyfiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.py)

foreach(pyfile ${pyfiles})
  configure_file(${pyfile} ${CMAKE_CURRENT_BINARY_DIR}/${pyfile} COPYONLY)
  get_filename_component(name ${pyfile} NAME)
  if(NOT name STREQUAL "conftest.py")
    add_test(NAME ${pyfile}
             COMMAND ${Python3_EXECUTABLE} -m pytest -v ${pyfile}
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endif()
endforeach()

if(PYTEST_NOT_FOUND)
  message(WARNING
    "pytest is not importable by ${Python3_EXECUTABLE}. The Python tests are "
    "registered but will fail until it is installed (pip install pytest).")
endif()

add_subdirectory(cdisort213)
