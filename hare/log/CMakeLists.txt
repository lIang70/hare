# Add hare_log's src
file(GLOB_RECURSE LOG_SOURCES
    "*.cc"
)

add_hare_library(hare_log
    INNER_LIBRARIES hare_base
    SOURCES ${LOG_SOURCES}
)