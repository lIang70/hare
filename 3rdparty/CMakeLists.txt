if(NOT TARGET FMT)
    add_subdirectory(fmt EXCLUDE_FROM_ALL)
    set(FMT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fmt/include"
        CACHE STRING
        "FMT include directory"
    )

    install(
        DIRECTORY "${FMT_INCLUDE_DIR}/fmt"
        DESTINATION include
    )
endif()