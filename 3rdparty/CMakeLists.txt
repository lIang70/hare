add_custom_target(autogen_jemalloc ALL
        COMMAND ./autogen.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc-5.3.0/
        COMMENT "Original jemalloc autogen target")
add_custom_target(build_jemalloc ALL
        COMMAND make -j
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc-5.3.0/
        COMMENT "Original jemalloc makefile target")
add_dependencies(build_jemalloc autogen_jemalloc)
set(JEMALLOC_LIBRARY ${CMAKE_SOURCE_DIR}/lib/libjemalloc.a)

add_library(jemalloc STATIC IMPORTED)
set_property(TARGET jemalloc APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(jemalloc PROPERTIES IMPORTED_LOCATION_NOCONFIG "${JEMALLOC_LIBRARY}")
add_dependencies(jemalloc build_jemalloc)