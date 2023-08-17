if(NOT TARGET jemalloc)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/jemalloc/" OR NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../lib/jemalloc/")
        add_custom_target(build_jemalloc ALL
            COMMAND ./autogen.sh && ./configure --prefix=$$PWD/../../lib/jemalloc && make -j$$(nproc) && make install
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc/
            COMMENT "Original jemalloc makefile target"
        )
    endif()
endif()

if(NOT TARGET GoogleTest)
    add_subdirectory(googletest)
endif()

if(NOT TARGET openssl)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/openssl/" OR NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../lib/openssl/")
        add_custom_target(build_openssl ALL
            COMMAND ./Configure shared --prefix=$$PWD/../../lib/openssl --openssldir=$$PWD/../../lib/openssl '-Wl,-rpath,$$(LIBRPATH)' && make -j$$(nproc) && make install
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/openssl/
            COMMENT "Original openssl makefile target"
        )
    endif()
endif()

if(NOT TARGET FMT)
    add_subdirectory(fmt EXCLUDE_FROM_ALL)
endif()