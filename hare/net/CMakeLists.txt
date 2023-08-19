project(hare_net
    VERSION ${HARE_VERSION_MAJOR}.${HARE_VERSION_MINOR}.${HARE_VERSION_PATCH}
    LANGUAGES CXX
)

# Add hare_net's src
file(GLOB PROJECT_SOURCES
    "*.cc"
    "tcp/*.cc"
)

# Add the source code to the project's library.
add_library(${PROJECT_NAME} ${HARE__LIBRARY_TYPE} ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${HARE_INCLUDE_DIR}/hare/net/)

if(JEMALLOC_FOUND)
    target_link_libraries(${PROJECT_NAME} hare_base ${OPENSSL_LIBRARIES} PkgConfig::Jemalloc)
else()
    target_link_libraries(${PROJECT_NAME} hare_base ${OPENSSL_LIBRARIES})
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/EHsc")
endif()

if(HARE__BUILD_TESTS)
    add_subdirectory(tests)
    add_subdirectory(samples)
endif(HARE__BUILD_TESTS)