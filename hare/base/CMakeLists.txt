# Add hare_base's src
file(GLOB_RECURSE BASE_SOURCES
    "*.cc"
)

if(JEMALLOC_FOUND)
    add_hare_library(hare_base
        LIBRARIES PkgConfig::Jemalloc
        SOURCES ${BASE_SOURCES}
    )
else()
    add_hare_library(hare_base
        SOURCES ${BASE_SOURCES}
    )
endif()