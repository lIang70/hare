# Add hare_log's src
file(GLOB LOG_SOURCES
    "*.cc"
    "backends/*.cc"
    "details/*.cc"
)

if(JEMALLOC_FOUND)
    add_hare_library(hare_log
        INNER_LIBRARIES hare_base
        LIBRARIES PkgConfig::Jemalloc
        SOURCES ${LOG_SOURCES}
    )
else()
    add_hare_library(hare_log
        INNER_LIBRARIES hare_base
        SOURCES ${LOG_SOURCES}
    )
endif()

if(HARE__BUILD_TESTS)
    add_subdirectory(tests)
endif(HARE__BUILD_TESTS)