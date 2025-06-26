# Install disrot library
#
#   library: libdisort_debug.so
#   library: libdisort_release.so
#
# Define the following symbols
#
#   DISORT_INCLUDE_DIR
#   pydisort::disort
#   pydisort::disort_cu
#
# Normal usage would be:
#
#   include_directories(${DISORT_INCLUDE_DIR})
#   target_link_libraries(pydisort::disort)


include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
  ${CMAKE_BINARY_DIR}
  SYSTEM ${TORCH_API_INCLUDE_DIR}
  SYSTEM ${TORCH_INCLUDE_DIR}
  )

string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

set(namel disort)
string(TOUPPER ${namel} nameu)

file(GLOB src_files *.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND src_files utils/DispatchStub.cpp)
endif()

add_library(${namel}_${buildl}
    SHARED
    ${src_files}
    )

set_target_properties(${namel}_${buildl}
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS_${buildu}}"
    )

target_include_directories(${namel}_${buildl}
    PRIVATE "${FMT_INCLUDE_DIR}"
    )

target_link_libraries(${namel}_${buildl}
    PUBLIC
    pydisort::cdisort
    fmt::fmt
    ${TORCH_LIBRARY}
    ${TORCH_CPU_LIBRARY}
    ${C10_LIBRARY}
    )

add_library(pydisort::disort ALIAS ${namel}_${buildl})

# CUDA source codes
if (CUDAToolkit_FOUND)
  file(GLOB cu_src_files *.cu)

  add_library(${namel}_cuda_${buildl}
      SHARED
      ${cu_src_files}
      )

  set_target_properties(${namel}_cuda_${buildl}
      PROPERTIES
      CUDA_SEPARABLE_COMPILATION ON
      )

  target_link_libraries(${namel}_cuda_${buildl}
      PUBLIC
      $<IF:$<BOOL:${CUDAToolkit_FOUND}>,CUDA::cudart,>
      $<IF:$<BOOL:${CUDAToolkit_FOUND}>,CUDA::cudart_static,>
      $<IF:$<BOOL:${CUDAToolkit_FOUND}>,${TORCH_CUDA_LIBRARY},>
      $<IF:$<BOOL:${CUDAToolkit_FOUND}>,${C10_CUDA_LIBRARY},>)

  add_library(pydisort::disort_cu ALIAS ${namel}_cuda_${buildl})
endif()

set(DISORT_INCLUDE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
  CACHE INTERNAL "disort include directory")
