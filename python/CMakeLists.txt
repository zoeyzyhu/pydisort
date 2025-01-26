# Installs pydiosrt module

set(namel pydisort)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

file(GLOB src_files
  pydisort.cpp
  )

# Set module targets
pybind11_add_module(${namel} MODULE ${src_files})

#add_library(${namel} MODULE ${src_files})

target_include_directories(${namel}
  PRIVATE ${DISORT_INCLUDE_DIR}
  SYSTEM ${TORCH_API_INCLUDE_DIR}
  SYSTEM ${TORCH_INCLUDE_DIR}
  )

set_target_properties(${namel}
  PROPERTIES
  CUDA_SEPARABLE_COMPILATION ON
  LINKER_LANGUAGE CUDA
  )

# -fvisibility=hidden is required to allow multiple modules compiled against
# different pybind versions to work properly, and for some features (e.g.
# py::module_local).  We force it on everything inside the `pybind11`
# namespace; also turning it on for a pybind module compilation here avoids
# potential warnings or issues from having mixed hidden/non-hidden types.
if(NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)
  set_target_properties(${target_name} PROPERTIES CXX_VISIBILITY_PRESET "hidden")
endif()

if(NOT DEFINED CMAKE_CUDA_VISIBILITY_PRESET)
  set_target_properties(${target_name} PROPERTIES CUDA_VISIBILITY_PRESET "hidden")
endif()

target_compile_definitions(${namel}
  PRIVATE
  __CUDA_NO_HALF_OPERATORS__
  __CUDA_NO_HALF_CONVERSIONS__
  __CUDA_NO_BFLOAT16_CONVERSIONS__
  __CUDA_NO_HALF2_OPERATORS__
  TORCH_API_INCLUDE_EXTENSION_H
  )

target_link_libraries(${namel}
  PRIVATE
  pydisort::disort
  pydisort::disort_cu
  pybind11::module
  ${TORCH_LIBRARY}
  ${TORCH_CPU_LIBRARY}
  ${TORCH_CUDA_LIBRARY}
  ${C10_LIBRARY}
  ${C10_CUDA_LIBRARY}
  ${TORCH_PYTHON_LIBRARY}
  CUDA::cudart
  )

set_target_properties(${namel} PROPERTIES
  PREFIX "${PYTHON_MODULE_PREFIX}"
  SUFFIX "${PYTHON_MODULE_EXTENSION}"
  )
