if(NOT TARGET jemalloc)
    set(JEMALLOC_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc/include)
    set(JEMALLOC_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc/lib/libjemalloc.so.2)
    add_custom_target(build_jemalloc ALL
        COMMAND_EXPAND_LISTS
        COMMAND ./autogen.sh && make -j$$(nproc)
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc/
        COMMENT "Original jemalloc makefile target"
        COMMAND ${CMAKE_COMMAND} -E
        make_directory "${CMAKE_SOURCE_DIR}/lib"
        COMMAND ${CMAKE_COMMAND} -E
        copy_if_different "${JEMALLOC_LIBRARY}" "${CMAKE_SOURCE_DIR}/lib/"
    )

    add_library(jemalloc SHARED IMPORTED GLOBAL)
    set_target_properties(jemalloc PROPERTIES IMPORTED_LOCATION ${JEMALLOC_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${JEMALLOC_INCLUDE})
endif()

if(NOT TARGET GoogleTest)
    add_subdirectory(googletest)
endif()