# ==========================================
# Compile the following libraries: - cdisort: via cmake instead of makefile
# ==========================================

# Add cdisort

add_subdirectory(cdisort)

set(CDISORT_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE PATH "cdisort include directory")

# Add cppdisort

add_subdirectory(cppdisort)

set(CPPDISORT_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE PATH "cppdisort include directory")
