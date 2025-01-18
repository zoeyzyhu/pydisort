include(FetchContent)
set(FETCHCONTENT_QUIET TRUE)

set(PACKAGE_NAME pybind11)
set(REPO_URL "https://github.com/pybind/pybind11")
set(REPO_TAG "v2.11.1")

add_package(${PACKAGE_NAME} ${REPO_URL} ${REPO_TAG} "" ON)
