add_custom_target(autogen_jemalloc ALL
        COMMAND ./autogen.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc/
        COMMENT "Original jemalloc autogen target")

add_custom_target(build_jemalloc ALL
        COMMAND make
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc/
        COMMENT "Original jemalloc makefile target")
add_dependencies(build_jemalloc autogen_jemalloc)

set(JEMALLOC_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc/include)
set(JEMALLOC_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc/lib/libjemalloc.so.2)

add_library(jemalloc STATIC IMPORTED GLOBAL)
set_property(TARGET jemalloc APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(jemalloc PROPERTIES IMPORTED_LOCATION_NOCONFIG ${JEMALLOC_LIBRARY}
                                          INTERFACE_INCLUDE_DIRECTORIES ${JEMALLOC_INCLUDE})
add_dependencies(jemalloc build_jemalloc)

if (NOT TARGET GoogleTest)
    add_subdirectory(googletest)
endif()