# =============================================
# Wrap the C++ library and bind to Python
# =============================================

set(namel pydisort)
file(GLOB src_files pybind_cppdisort.cpp)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

# Set module targets

pybind11_add_module(${namel} MODULE ${src_files})

target_include_directories(
  ${namel}
  PRIVATE ${PYDISORT_INCLUDE_DIR} ${CMAKE_BINARY_DIR} ${TOMLPP_INCLUDE_DIR}
          ${CDISORT_INCLUDE_DIR} ${PYTHON_INCLUDE_DIR} ${PYBIND11_INCLUDE_DIR})

target_link_libraries(${namel} PRIVATE ${PYDISORT_LIBRARY_${buildu}}
                                       ${PYTHON_LIBRARY_RELEASE})

set_target_properties(${namel} PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}"
                                          SUFFIX "${PYTHON_MODULE_EXTENSION}")

# Install configuration

install(
  TARGETS ${namel}
  COMPONENT python
  LIBRARY DESTINATION ${namel})
