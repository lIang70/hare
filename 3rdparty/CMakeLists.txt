if(NOT TARGET jemalloc)
    add_custom_target(build_jemalloc ALL
        COMMAND ./autogen.sh && ./configure --prefix=$$PWD/../../lib/jemalloc && make -j$$(nproc) && make install
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc/
        COMMENT "Original jemalloc makefile target"
    )

    set(JEMALLOC_INCLUDE ${CMAKE_SOURCE_DIR}/lib/jemalloc/include)
    set(JEMALLOC_LIBRARY ${CMAKE_SOURCE_DIR}/lib/jemalloc/lib/libjemalloc.so.2)
    add_library(jemalloc SHARED IMPORTED GLOBAL)
    set_target_properties(jemalloc PROPERTIES
        IMPORTED_LOCATION ${JEMALLOC_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${JEMALLOC_INCLUDE})
endif()

if(NOT TARGET GoogleTest)
    add_subdirectory(googletest)
endif()

if(NOT TARGET openssl)
    add_custom_target(build_openssl ALL
        COMMAND ./Configure shared --prefix=$$PWD/../../lib/openssl --openssldir=$$PWD/../../lib/openssl '-Wl,-rpath,$$(LIBRPATH)' && make -j$$(nproc) && make install
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/openssl/
        COMMENT "Original openssl makefile target"
    )
endif()