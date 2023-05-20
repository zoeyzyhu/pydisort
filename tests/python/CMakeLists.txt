
# Copy python test files to build directory

file(GLOB pyfiles *.py)

foreach(pyfile ${pyfiles})
    get_filename_component(name ${pyfile} NAME)
    configure_file(${pyfile}
        ${CMAKE_BINARY_DIR}/bin/${name} COPYONLY
        )
endforeach()
