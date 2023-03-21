if(NOT TARGET jemalloc)
    set(JEMALLOC_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc/include)
    set(JEMALLOC_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc/lib/libjemalloc.so.2)
    add_custom_target(build_jemalloc ALL
        COMMAND ./autogen.sh && ./configure --prefix=$$PWD/../../build/3rdparty/jemalloc && make -j$$(nproc) && make install
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc/
        COMMENT "Original jemalloc makefile target"
    )
endif()

if(NOT TARGET GoogleTest)
    add_subdirectory(googletest)
endif()

if(NOT TARGET openssl)
    add_custom_target(build_openssl ALL
        COMMAND ./Configure shared --prefix=$$PWD/../../build/3rdparty/openssl --openssldir=$$PWD/../../build/openssl '-Wl,-rpath,$$(LIBRPATH)' && make -j$$(nproc) && make install
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/openssl/
        COMMENT "Original openssl makefile target"
    )
endif()