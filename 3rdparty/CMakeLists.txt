set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

set(JEMALLOC_LIBRARY ${CMAKE_SOURCE_DIR}/lib/libjemalloc.a)
message(${CMAKE_MAKE_PROGRAM})
add_custom_target(autogen_jemalloc ALL
        COMMAND configure
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc-5.3.0/
        COMMENT "Original jemalloc makefile target")

add_library(jemalloc STATIC IMPORTED)
set_property(TARGET jemalloc APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(jemalloc PROPERTIES IMPORTED_LOCATION_NOCONFIG "${JEMALLOC_LIBRARY}")
add_dependencies(jemalloc autogen_jemalloc)