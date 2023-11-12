# =============================================
# Wrap the C++ library and bind to Python
# =============================================

set(namel pydisort)
file(GLOB src_files pydisort.cpp)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

# Set module targets

pybind11_add_module(${namel} MODULE ${src_files})

target_include_directories(
  ${namel}
  PRIVATE ${PYDISORT_INCLUDE_DIR} ${CMAKE_BINARY_DIR} ${TOMLPP_INCLUDE_DIR}
          ${CDISORT_INCLUDE_DIR} ${PYTHON_INCLUDE_DIR} ${PYBIND11_INCLUDE_DIR})

target_link_libraries(${namel} PRIVATE ${CPPDISORT_LIBRARY_${buildu}} pybind11::module)

set_target_properties(${namel} PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}"
                                          SUFFIX "${PYTHON_MODULE_EXTENSION}")

# Execute a Python script to get the site-packages path
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import site; print(site.getsitepackages()[0])"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

install(
  TARGETS ${namel}
  COMPONENT python
  LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
