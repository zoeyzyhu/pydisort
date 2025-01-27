# A small macro used for setting up the build of a test.
#
# Usage: setup_test(name)

string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

macro(setup_test namel)
  add_executable(${namel}.${buildl} ${namel}.cpp)

  set_target_properties(${namel}.${buildl}
                        PROPERTIES COMPILE_FLAGS ${CMAKE_CXX_FLAGS_${buildu}})

  target_include_directories(
    ${namel}.${buildl}
    PRIVATE ${CMAKE_BINARY_DIR}
            ${DISORT_INCLUDE_DIR}
            SYSTEM
            ${TORCH_INCLUDE_DIR}
            SYSTEM
            ${TORCH_API_INCLUDE_DIR})

  target_link_libraries(${namel}.${buildl}
    PRIVATE pydisort::disort
            ${TORCH_LIBRARY}
            ${TORCH_CPU_LIBRARY}
            ${C10_LIBRARY}
            $<IF:$<BOOL:${CUDAToolkit_FOUND}>,pydisort::disort_cu,>
            $<IF:$<BOOL:${CUDAToolkit_FOUND}>,${TORCH_CUDA_LIBRARY},>
            $<IF:$<BOOL:${CUDAToolkit_FOUND}>,${C10_CUDA_LIBRARY},>)

  add_test(NAME ${namel}.${buildl} COMMAND ${namel}.${buildl})
endmacro()
