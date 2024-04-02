# Installs pydiosrt module

set(namel pydisort)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

file(GLOB src_files 
    pydisort.cpp
    )

# Set module targets

pybind11_add_module(${namel} MODULE ${src_files})

target_include_directories(${namel}
  PRIVATE
  ${PYDISORT_INCLUDE_DIR}
  )

target_link_libraries(${namel}
  PRIVATE
  pydisort::disort++
  pybind11::module
  )

set_target_properties(${namel}
  PROPERTIES 
  PREFIX "${PYTHON_MODULE_PREFIX}"
  SUFFIX "${PYTHON_MODULE_EXTENSION}"
  )
