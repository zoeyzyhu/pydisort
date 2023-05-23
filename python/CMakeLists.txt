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
	${PYTHON_INCLUDE_DIR}
	${PYBIND11_INCLUDE_DIR}
	)

target_link_libraries(${namel}
	PRIVATE
	${CPPDISORT_LIBRARY_${buildu}}
    	${PYTHON_LIBRARY_RELEASE}
	)

install(TARGETS ${namel}
	COMPONENT python
	LIBRARY DESTINATION "${PYTHON_SITE_PACKAGES}"
	)
