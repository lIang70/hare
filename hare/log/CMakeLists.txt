# Add hare_log's src
file(GLOB_RECURSE LOG_SOURCES
    "*.cc"
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