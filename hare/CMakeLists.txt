# Add header
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include)

# Add hare_base's src
file(GLOB HARE_BASE_SOURCES "base/*.cc"
                            "base/log/*.cc"
                            "base/thread/*.cc"
                            "base/time/*.cc"
                            "base/util/*.cc")

# Add the source code to the project's executable。
add_library(hare_base ${HARE__LIBRARY_TYPE} ${HARE_BASE_SOURCES})

if(HARE__BUILD_TESTS)
    add_subdirectory(base/tests)
endif(HARE__BUILD_TESTS)

