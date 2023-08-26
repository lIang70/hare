project(echo_serve)

find_package(PkgConfig REQUIRED)

if(PKG_CONFIG_FOUND)
    pkg_search_module(Jemalloc REQUIRED IMPORTED_TARGET jemalloc>=5.3)
    set(JEMALLOC_FOUND ON)
else()
    set(JEMALLOC_FOUND OFF)
endif(PKG_CONFIG_FOUND)

file(GLOB SOURCE_FILE *.cc)

add_executable(${PROJECT_NAME} ${SOURCE_FILE})

if(JEMALLOC_FOUND)
    target_link_libraries(${PROJECT_NAME} hare_base hare_log hare_net PkgConfig::Jemalloc)
else()
    target_link_libraries(${PROJECT_NAME} hare_base hare_log hare_net)
endif()