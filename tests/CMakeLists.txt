# ============================
# A collection of unit tests
# ============================

enable_testing()

# C++ tests
setup_test(test_disort)

# Python tests

file(GLOB pyfiles *.py)

foreach(pyfile ${pyfiles})
  get_filename_component(name ${pyfile} NAME)
  message(STATUS "Copying ${pyfile} to ${name}")
  configure_file(${pyfile} ${CMAKE_CURRENT_BINARY_DIR}/${name} @ONLY)
  add_test(NAME ${name} COMMAND python3 ${name})
endforeach()

add_subdirectory(cdisort213)
