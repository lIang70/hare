# Add hare_base's src
file(GLOB BASE_SOURCES
    "*.cc"
    "time/*.cc"
    "util/*.cc"
    "io/*.cc"
    "io/reactor/*.cc"
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

if(HARE__BUILD_TESTS)
    add_subdirectory(tests)
endif(HARE__BUILD_TESTS)