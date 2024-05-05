# Setup for GCC compiler:
#
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(${PROJECT_NAME}_CXX_FLAGS
        "-g3"
        CACHE INTERNAL "${PROJECT_NAME} CXX compiler flags")
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(${PROJECT_NAME}_CXX_FLAGS
        "-O3"
        CACHE INTERNAL "${PROJECT_NAME} CXX compiler flags")
  else()
    message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}")
  endif()

  set(KNOWN_CXX_COMPILER TRUE)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(${PROJECT_NAME}_C_FLAGS
        "-g3"
        CACHE INTERNAL "${PROJECT_NAME} C compiler flags")
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(${PROJECT_NAME}_C_FLAGS
        "-O3"
        CACHE INTERNAL "${PROJECT_NAME} C compiler flags")
  else()
    message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}")
  endif()

  set(KNOWN_C_COMPILER TRUE)
endif()

# Setup for Clang compiler:
#
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(${PROJECT_NAME}_CXX_FLAGS
        "-g3"
        CACHE INTERNAL "${PROJECT_NAME} CXX compiler flags")
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(${PROJECT_NAME}_CXX_FLAGS
        "-O3"
        CACHE INTERNAL "${PROJECT_NAME} CXX compiler flags")
  else()
    message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}")
  endif()

  set(KNOWN_CXX_COMPILER TRUE)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(${PROJECT_NAME}_C_FLAGS
        "-g3"
        CACHE INTERNAL "${PROJECT_NAME} C compiler flags")
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(${PROJECT_NAME}_C_FLAGS
        "-O3"
        CACHE INTERNAL "${PROJECT_NAME} C compiler flags")
  else()
    message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}")
  endif()

  set(KNOWN_C_COMPILER TRUE)
endif()

# Setup for ICC compiler:
#
if(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(${PROJECT_NAME}_CXX_FLAGS
        "-g3"
        CACHE INTERNAL "${PROJECT_NAME} CXX compiler flags")
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(${PROJECT_NAME}_CXX_FLAGS
        "-O3"
        CACHE INTERNAL "${PROJECT_NAME} CXX compiler flags")
  else()
    message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}")
  endif()

  set(KNOWN_CXX_COMPILER TRUE)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Intel")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(${PROJECT_NAME}_C_FLAGS
        "-g3"
        CACHE INTERNAL "${PROJECT_NAME} C compiler flags")
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(${PROJECT_NAME}_C_FLAGS
        "-O3"
        CACHE INTERNAL "${PROJECT_NAME} C compiler flags")
  else()
    message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}")
  endif()

  set(KNOWN_C_COMPILER TRUE)
endif()

if(NOT KNOWN_CXX_COMPILER)
  message(FATAL_ERROR "\nUnknown C++ compiler!\n")
endif()

if(NOT KNOWN_C_COMPILER)
  message(FATAL_ERROR "\nUnknown C compiler!\n")
endif()
