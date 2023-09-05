# Add hare_base's src
file(GLOB_RECURSE BASE_SOURCES
    "*.cc"
)

add_hare_library(hare_base
    SOURCES ${BASE_SOURCES}
)