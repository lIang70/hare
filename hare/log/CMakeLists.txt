project(hare_log
    VERSION ${HARE_VERSION_MAJOR}.${HARE_VERSION_MINOR}.${HARE_VERSION_PATCH}
    LANGUAGES CXX
)

# Add hare_log's src
file(GLOB PROJECT_SOURCES
    "*.cc"
    "backends/*.cc"
    "details/*.cc"
)

# Add the source code to the project's library.
add_library(${PROJECT_NAME} ${HARE__LIBRARY_TYPE} ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC
    ${HARE_INCLUDE_DIR}/hare/log/
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/fmt/include/)

if(JEMALLOC_FOUND)
    target_link_libraries(${PROJECT_NAME} hare_base PkgConfig::Jemalloc)
else()
    target_link_libraries(${PROJECT_NAME} hare_base)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/EHsc")
endif()

if(HARE__BUILD_TESTS)
    add_subdirectory(tests)
endif(HARE__BUILD_TESTS)