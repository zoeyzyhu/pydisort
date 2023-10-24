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

set(PYDISORT_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE PATH "pydisort include directory")
