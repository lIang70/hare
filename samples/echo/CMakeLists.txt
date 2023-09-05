project(echo_serve)

file(GLOB SOURCE_FILE *.cc)

add_executable(${PROJECT_NAME} ${SOURCE_FILE})

target_link_libraries(${PROJECT_NAME} hare_base hare_log hare_net)