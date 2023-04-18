include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include)

file(GLOB TESTS_LIST *.cc)

foreach(test_path ${TESTS_LIST})
    get_filename_component(TARGET ${test_path} NAME_WE)
    get_filename_component(TARGET_FILE ${test_path} NAME)
    add_executable(${TARGET} ${test_path})
    target_link_libraries(${TARGET} hare_base hare_net)
    message( STATUS "Add test[${TARGET}] by [${TARGET_FILE}]")
endforeach()