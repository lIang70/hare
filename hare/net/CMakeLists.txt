# Add hare_net's src
file(GLOB_RECURSE NET_SOURCES
    "*.cc"
)

find_package(OpenSSL)
if(OPENSSL_FOUND)
    set(OPENSSL_TARGETS OpenSSL::SSL)

    message(STATUS "OpenSSL include: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL lib: ${OPENSSL_LIBRARIES}")
endif()

if(JEMALLOC_FOUND)
    add_hare_library(hare_net
        INNER_LIBRARIES hare_base
        LIBRARIES PkgConfig::Jemalloc ${OPENSSL_TARGETS}
        SOURCES ${NET_SOURCES}
    )
else()
    add_hare_library(hare_net
        INNER_LIBRARIES hare_base
        LIBRARIES ${OPENSSL_TARGETS}
        SOURCES ${NET_SOURCES}
    )
endif()