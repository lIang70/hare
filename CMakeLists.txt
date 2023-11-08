cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif(POLICY CMP0091)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release
        CACHE STRING "Set build type to Debug or Release (default Release)" FORCE)
    message(STATUS "Set CMAKE_BUILD_TYPE to Release (default)")
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)

project(hare
    LANGUAGES CXX
)

if(NOT HARE__BUILD_TESTS)
    option(HARE__BUILD_TESTS "Build all of own tests." ON)
endif()

set(GNUC 0)
set(CLANG 0)
set(MSVC 0)

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CLANG 1)
endif()

if(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") OR(${CLANG}))
    set(GNUC 1)
endif()

if(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC") OR("${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC"))
    set(MSVC 1)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
set(HARE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

include(CheckCXXCompilerFlag)
include(CheckFileSymbol)
include(VersionViaGit)

hare_fuzzy_version_from_git()

set(HARE_VERSION_MAJOR ${HARE_GIT__VERSION_MAJOR})
set(HARE_VERSION_MINOR ${HARE_GIT__VERSION_MINOR})
set(HARE_VERSION_PATCH ${HARE_GIT__VERSION_PATCH})
set(HARE_VERSION_STAGE ${HARE_GIT__VERSION_STAGE})

set(HARE_ABI_MAJOR ${HARE_VERSION_MAJOR})
set(HARE_ABI_MINOR ${HARE_VERSION_MINOR})
set(HARE_ABI_PATCH ${HARE_VERSION_PATCH})

set(HARE_ABI_LIBVERSION
    "${HARE_ABI_MAJOR}.${HARE_ABI_MINOR}.${HARE_ABI_PATCH}")

set(HARE_PACKAGE_VERSION
    "${HARE_VERSION_MAJOR}.${HARE_VERSION_MINOR}.${HARE_VERSION_PATCH}")

set(HARE_ABI_LIBVERSION_CURRENT 1)
set(HARE_ABI_LIBVERSION_REVISION 0)
set(HARE_ABI_LIBVERSION_AGE 0)

set(HARE_PACKAGE_RELEASE "${HARE_VERSION_MAJOR}.${HARE_VERSION_MINOR}")

if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()

if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()

if(NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)

if(NOT HARE__LIBRARY_TYPE)
    set(HARE__LIBRARY_TYPE SHARED CACHE STRING "Set library type to SHARED/STATIC")
endif()

set(HARE_LIBRARY_STATIC OFF)
set(HARE_LIBRARY_SHARED OFF)

if(${HARE__LIBRARY_TYPE} MATCHES "SHARED")
    set(HARE_LIBRARY_SHARED ON)
    add_definitions(-DHARE_SHARED)
else()
    set(HARE_LIBRARY_STATIC ON)
    add_definitions(-DHARE_STATIC)
endif()

message(STATUS "HARE__LIBRARY_TYPE: ${HARE__LIBRARY_TYPE}")

macro(add_compiler_flags)
    foreach(flag ${ARGN})
        string(REGEX REPLACE "[-.+/:= ]" "_" _flag_esc "${flag}")

        check_cxx_compiler_flag("${flag}" check_cxx_compiler_flag_${_flag_esc})

        if(check_cxx_compiler_flag_${_flag_esc})
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
        endif()
    endforeach()
endmacro()

if(CMAKE_BUILD_TYPE_LOWER STREQUAL "debug")
    add_definitions(-DHARE_DEBUG)
    add_compiler_flags(-g -O0)
    add_compiler_flags(-fsanitize=address -fsanitize-recover=address -fno-stack-protector -fno-omit-frame-pointer)
else()
    add_compiler_flags(-O2)
endif()

if(${MSVC})
    set(msvc_static_runtime OFF)

    if("${HARE_LIBRARY_TYPE}" STREQUAL "STATIC")
        set(msvc_static_runtime ON)
    endif()

    # For more info:
    # - https://docs.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library?view=vs-2017
    # - https://gitlab.kitware.com/cmake/community/wikis/FAQ#how-can-i-build-my-msvc-application-with-a-static-runtime
    option(HARE__MSVC_STATIC_RUNTIME
        "Link static runtime libraries"
        ${msvc_static_runtime})

    if(HARE__MSVC_STATIC_RUNTIME)
        foreach(flag_var
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
        )
            if(${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif()
        endforeach()
    endif()
endif()

# GNUC specific options.
if(${GNUC})
    option(HARE__DISABLE_GCC_WARNINGS "Disable verbose warnings with GCC" OFF)
    option(HARE__ENABLE_GCC_HARDENING "Enable compiler security checks" OFF)
    option(HARE__ENABLE_GCC_FUNCTION_SECTIONS "Enable gcc function sections" OFF)
    option(HARE__ENABLE_GCC_WARNINGS "Make all GCC warnings into errors" OFF)

    list(APPEND __FLAGS
        -Wall -Wextra -Wno-unused-parameter -Wstrict-aliasing -Wstrict-prototypes
        -Wundef

        -Wmissing-prototypes

        -Wwrite-strings

        # Disable unused-function warnings
        -Wno-unused-function

        -Wno-pragmas

        -Wvla
    )

    if(HARE__DISABLE_GCC_WARNINGS)
        list(APPEND __FLAGS -w)
    endif()

    if(HARE__ENABLE_GCC_HARDENING)
        list(APPEND __FLAGS
            -fstack-protector-all
            -fwrapv
            -fPIE
            -Wstack-protector
            "--param ssp-buffer-size=1")

        add_definitions(-D_FORTIFY_SOURCE=3)
    endif()

    if(HARE__ENABLE_GCC_FUNCTION_SECTIONS)
        list(APPEND __FLAGS -ffunction-sections)

        # TODO: Add --gc-sections support. We need some checks for NetBSD to ensure this works.
    endif()

    if(HARE__ENABLE_GCC_WARNINGS)
        list(APPEND __FLAGS -Werror)
    endif()

    add_compiler_flags(${__FLAGS})
endif()

if(MINGW OR CYGWIN)
    set(WIN32 TRUE)
endif()

# Winsock.
if(WIN32)
    list(APPEND CMAKE_REQUIRED_LIBRARIES
        iphlpapi
        ws2_32
    )
    set(CMAKE_REQUIRED_DEFINITIONS -FIwinsock2.h -FIws2tcpip.h -Fiphlpapi.h)
endif()

if(${GNUC})
    set(HARE_SHARED_FLAGS -fvisibility=hidden)
endif()

add_subdirectory(3rdparty)
add_subdirectory(hare)

if(NOT HARE__BUILD_SAMPLES)
    option(HARE__BUILD_SAMPLES "Build all of own tests." ON)
endif()

if(HARE__BUILD_SAMPLES)
    add_subdirectory(samples)
endif(HARE__BUILD_SAMPLES)