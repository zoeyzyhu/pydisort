# ============================
# A collection of unit tests
# ============================

add_subdirectory(cdisort)
add_subdirectory(cppdisort)
add_subdirectory(python)

# Copy input files to run directory

file(GLOB inputs *.toml)
foreach(input ${inputs})
	get_filename_component(name ${input} NAME)
  	configure_file(${input}
		${CMAKE_BINARY_DIR}/bin/${name} COPYONLY
		)
endforeach()
