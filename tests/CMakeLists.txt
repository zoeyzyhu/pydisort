# ============================
# A collection of unit tests
# ============================


add_subdirectory(cdisort)
add_subdirectory(cppdisort)

# Copy python test files to build directory

file(GLOB pyfiles python/*.py)

foreach(pyfile ${pyfiles})
    get_filename_component(name ${pyfile} NAME)
    configure_file(${pyfile}
        ${CMAKE_BINARY_DIR}/bin/${name} COPYONLY
        )
endforeach()
