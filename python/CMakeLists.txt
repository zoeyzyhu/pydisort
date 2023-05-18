# =============================================
# Wrap the C++ library and bind to Python
# =============================================

set(namel pydisort)
file(GLOB src_files
	pybind_cppdisort.cpp
	)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

pybind11_add_module(${namel}
	MODULE
	${src_files}
	)

target_include_directories(${namel}
	PRIVATE 
	${CPPDISORT_INCLUDE_DIR}
	${CMAKE_BINARY_DIR}
	${TOMLPP_INCLUDE_DIR}
    ${CDISORT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${PYBIND11_INCLUDE_DIR}
	)

target_link_libraries(${namel}
	PRIVATE
	${CPPDISORT_LIBRARY_${buildu}}
	${PYTHON_LIBRARIES}
	)

execute_process(
	COMMAND "${PYTHON_EXECUTABLE}" -c "import sysconfig; print(sysconfig.get_paths()['purelib'])"
	OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
	OUTPUT_STRIP_TRAILING_WHITESPACE
	)

install(TARGETS ${namel}
	COMPONENT python
	LIBRARY DESTINATION "${PYTHON_SITE_PACKAGES}"
	)
