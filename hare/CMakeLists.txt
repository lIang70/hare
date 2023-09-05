# Check if header files exist
list(APPEND FILES_TO_CHECK
    fcntl.h
    inttypes.h
    memory.h
    signal.h
    stdarg.h
    stddef.h
    stdint.h
    stdlib.h
    string.h
    errno.h
    unistd.h
    time.h
    sys/types.h
    sys/stat.h
    sys/time.h
    sys/param.h
)

if(WIN32)
    list(APPEND FILES_TO_CHECK
        io.h
        winsock2.h
        ws2tcpip.h
        afunix.h
    )
else()
    list(APPEND FILES_TO_CHECK
        netdb.h
        dlfcn.h
        arpa/inet.h
        poll.h
        port.h
        sys/socket.h
        sys/random.h
        sys/un.h
        sys/devpoll.h
        sys/epoll.h
        sys/eventfd.h
        sys/event.h
        sys/ioctl.h
        sys/mman.h
        sys/queue.h
        sys/select.h
        sys/sendfile.h
        sys/uio.h
        sys/wait.h
        sys/resource.h
        sys/timerfd.h
        sys/signalfd.h
        netinet/in.h
        netinet/in6.h
        netinet/tcp.h
        ifaddrs.h
    )
endif()

if(NOT "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
    list(APPEND FILES_TO_CHECK sys/sysctl.h)
endif()

# Fills HARE_INCLUDES
foreach(FILE ${FILES_TO_CHECK})
    CHECK_INCLUDE_FILE_CONCAT(${FILE} "HARE")
endforeach()

unset(FILES_TO_CHECK)

# Check if symbol exist
list(APPEND SYMBOLS_TO_CHECK
    getaddrinfo
    getnameinfo
    getprotobynumber
    getservbyname
    gethostbyname
    inet_ntop
    inet_pton
    gettimeofday
    signal
    strtoll
    strlcpy
    strsep
    strtok_r
    timerclear
    timercmp
    timerisset
    timeradd
    nanosleep
    putenv
    umask
)

if(WIN32)
    list(APPEND SYMBOLS_TO_CHECK
        _gmtime64_s
        _gmtime64
    )
else()
    list(APPEND SYMBOLS_TO_CHECK
        getifaddrs
        select
        epoll_create
        epoll_create1
        epoll_pwait2
        epoll_ctl
        eventfd
        poll
        port_create
        kqueue
        fcntl
        mmap
        pipe
        pipe2
        pread
        sendfile
        sigaction
        strsignal
        sysctl
        accept4
        arc4random
        arc4random_buf
        arc4random_addrandom
        getrandom
        getegid
        geteuid
        issetugid
        usleep
        timerfd_create
        setenv
        unsetenv
        setrlimit
        gethostbyname_r
        mmap64
    )
endif()

list(APPEND CMAKE_EXTRA_INCLUDE_FILES ${HARE_INCLUDES} stdio.h)
CHECK_SYMBOLS_EXIST("${SYMBOLS_TO_CHECK}" "${CMAKE_EXTRA_INCLUDE_FILES}" "HARE")
unset(SYMBOLS_TO_CHECK)
set(HARE__HAVE_EPOLL ${HARE__HAVE_EPOLL_CREATE})
set(HARE__HAVE_SIGNALFD ${HARE__HAVE_SYS_SIGNALFD_H})

if(WIN32 AND NOT CYGWIN)
    set(HARE__HAVE_WEPOLL 1)
endif()

if(HAVE_PORT_H AND HAVE_PORT_CREATE)
    set(HARE__HAVE_HARE_PORTS 1)
endif()

if(HARE__HAVE_EPOLL)
    list(APPEND BACKENDS EPOLL)
endif()

if(HARE__HAVE_SELECT)
    list(APPEND BACKENDS SELECT)
endif()

if(HARE__HAVE_POLL)
    list(APPEND BACKENDS POLL)
endif()

if(HARE__HAVE_WEPOLL)
    list(APPEND BACKENDS WEPOLL)
endif()

if(WIN32)
    list(APPEND BACKENDS WIN32)
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/hare-config.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/include/hare/hare-config.h
    NEWLINE_STYLE UNIX)

# Add hare-config
include_directories(${HARE_INCLUDE_DIR} ${FMT_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(${JEMALLOC_INCLUDE_DIRS})

if(WIN32)
    set(LIB_PLATFORM iphlpapi ws2_32)
endif()

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
    find_package(GTest CONFIG REQUIRED)
endif(HARE__BUILD_TESTS)

set(LIBHARE_SHARED_LIBRARIES "")
set(LIBHARE_STATIC_LIBRARIES "")

include(AddHareLibrary)

add_subdirectory(base)
add_subdirectory(log)
add_subdirectory(net)

if(HARE__BUILD_TESTS)
    add_subdirectory(tests)
endif(HARE__BUILD_TESTS)

#
# Installation preparation.
#
set(HARE_INSTALL_CMAKE_DIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/libhare")

export(PACKAGE libhare)

configure_file(${PROJECT_SOURCE_DIR}/cmake/LibhareConfig.cmake.in
    ${PROJECT_BINARY_DIR}/LibhareConfig.cmake
    @ONLY)

# Generate version info for both build-tree and install-tree.
configure_file(${PROJECT_SOURCE_DIR}/cmake/LibhareConfigVersion.cmake.in
    ${PROJECT_BINARY_DIR}/LibhareConfigVersion.cmake
    @ONLY)

# Install public headers
install(
    DIRECTORY ${HARE_INCLUDE_DIR}/hare
    DESTINATION "include"
    COMPONENT dev
)

# Install the configs.
install(FILES
    ${PROJECT_BINARY_DIR}/LibhareConfig.cmake
    ${PROJECT_BINARY_DIR}/LibhareConfigVersion.cmake
    DESTINATION "${HARE_INSTALL_CMAKE_DIR}"
    COMPONENT dev)

# Install exports for the install-tree.
macro(install_export type)
    install(EXPORT LibhareTargets-${type}
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION "${HARE_INSTALL_CMAKE_DIR}"
        COMPONENT dev)
endmacro()

if(${HARE_LIBRARY_STATIC})
    install_export(static)
endif()

if(${HARE_LIBRARY_SHARED})
    install_export(shared)
endif()

if(NOT TARGET uninstall)
    # Create the uninstall target.
    # https://gitlab.kitware.com/cmake/community/wikis/FAQ#can-i-do-make-uninstall-with-cmake
    configure_file(${PROJECT_SOURCE_DIR}/cmake/Uninstall.cmake.in
        ${PROJECT_BINARY_DIR}/Uninstall.cmake
        @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${PROJECT_BINARY_DIR}/Uninstall.cmake)
endif()

message(STATUS "")
message(STATUS "        ---( Libhare " ${HARE_PACKAGE_VERSION} " )---")
message(STATUS "")
message(STATUS "Available event backends: ${BACKENDS}")
message(STATUS "CMAKE_BINARY_DIR:         ${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "CMAKE_SOURCE_DIR:         ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "PROJECT_BINARY_DIR:       ${PROJECT_BINARY_DIR}")
message(STATUS "PROJECT_SOURCE_DIR:       ${PROJECT_SOURCE_DIR}")
message(STATUS "CMAKE_MODULE_PATH:        ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_COMMAND:            ${CMAKE_COMMAND}")
message(STATUS "CMAKE_ROOT:               ${CMAKE_ROOT}")
message(STATUS "CMAKE_SYSTEM:             ${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SYSTEM_NAME:        ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_VERSION:     ${CMAKE_SYSTEM_VERSION}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR:   ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SKIP_RPATH:         ${CMAKE_SKIP_RPATH}")
message(STATUS "CMAKE_SKIP_INSTALL_RPATH: ${CMAKE_SKIP_INSTALL_RPATH}")
message(STATUS "CMAKE_INSTALL_RPATH:      ${CMAKE_INSTALL_RPATH}")
message(STATUS "CMAKE_VERBOSE_MAKEFILE:   ${CMAKE_VERBOSE_MAKEFILE}")
message(STATUS "CMAKE_CXX_FLAGS:          ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_BUILD_TYPE:         ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_COMPILER:       ${CMAKE_CXX_COMPILER} (id ${CMAKE_CXX_COMPILER_ID}, CLANG ${CLANG}, GNUC ${GNUC}, version ${CMAKE_CXX_COMPILER_VERSION})")
message(STATUS "CMAKE_AR:                 ${CMAKE_AR}")
message(STATUS "CMAKE_RANLIB:             ${CMAKE_RANLIB}")
message(STATUS "CMAKE_INSTALL_PREFIX:     ${CMAKE_INSTALL_PREFIX}")
message(STATUS "CMAKE_DEBUG_POSTFIX:      ${CMAKE_DEBUG_POSTFIX}")
message(STATUS "")