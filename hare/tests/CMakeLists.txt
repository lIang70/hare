include_directories(${HARE_INCLUDE_DIR} ${FMT_INCLUDE_DIR})

file(GLOB TESTS_LIST *.cc)

foreach(test_path ${TESTS_LIST})
    get_filename_component(TARGET ${test_path} NAME_WE)
    get_filename_component(TARGET_FILE ${test_path} NAME)
    add_executable(${TARGET} ${test_path})
    target_link_libraries(${TARGET} GTest::gtest_main hare_base hare_log hare_net)
    gtest_discover_tests(${TARGET})
endforeach()
