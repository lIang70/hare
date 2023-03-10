cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(hare VERSION 0.1)

message(STATUS "CMAKE_C_COMPILER_ID = ${CMAKE_C_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_HOST_SYSTEM: ${CMAKE_HOST_SYSTEM}")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)

if(NOT HARE__LIBRARY_TYPE)
    set(HARE__LIBRARY_TYPE SHARED CACHE STRING "Set library type to SHARED/STATIC")
endif()

message(STATUS "HARE__LIBRARY_TYPE: ${HARE__LIBRARY_TYPE}")

if(NOT HARE__BUILD_TESTS)
    option(HARE__BUILD_TESTS "Build all of own tests." ON)
endif()

if(NOT BUILD_WITH_DEBUGINFO)
    option(BUILD_WITH_DEBUGINFO "Build with DEBUG-INFO" OFF)
endif()

if(BUILD_WITH_DEBUGINFO)
    add_definitions(-DHARE_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb -pthread")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize-recover=address -fno-stack-protector -fno-omit-frame-pointer")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O0 /DEBUG /Zi /EHa")
    endif()
else()
    set(CMAKE_BUILD_TYPE MinSizeRel)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -pthread")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /EHa")
    endif()
endif()

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

add_subdirectory(3rdparty)

if(HARE__BUILD_TESTS)
    if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        if(MSVC)
            # BigObj required for tests.
            set(CXX_FLAGS "${CXX_FLAGS} -bigobj")
        endif(MSVC)
    endif()

    # To enable test
    enable_testing()
    include(GoogleTest)
endif(HARE__BUILD_TESTS)

add_subdirectory(hare)