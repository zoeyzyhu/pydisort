# Copy python test files to build directory

file(GLOB pyfiles *.py)

foreach(pyfile ${pyfiles})
  get_filename_component(name ${pyfile} NAME)
  configure_file(${pyfile} ${CMAKE_CURRENT_BINARY_DIR}/tests/python/${name} COPYONLY)
  add_test(NAME ${pyfile} COMMAND "python3 ${pyfile}")
endforeach()
