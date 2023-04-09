include(macors.cmake)

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

# Fills EVENT_INCLUDES
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
    set(HARE__HAVE_EVENT_PORTS 1)
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

message(STATUS "Available event backends: ${BACKENDS}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/hare-config.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/include/hare/hare-config.h
    NEWLINE_STYLE UNIX)

find_package(PkgConfig REQUIRED)

if(EXISTS "${CMAKE_SOURCE_DIR}/lib/jemalloc/" AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/jemalloc/")
    message(STATUS "Jemalloc is built")
    if(PKG_CONFIG_FOUND)
        set(ENV{PKG_CONFIG_PATH} ${CMAKE_SOURCE_DIR}/lib/jemalloc/lib/pkgconfig:$ENV{PKG_CONFIG_PATH})
        pkg_search_module(Jemalloc REQUIRED IMPORTED_TARGET jemalloc>=5.3)
        option(JEMALLOC_FOUND ON)
    endif(PKG_CONFIG_FOUND)
else()
    message(STATUS "Jemalloc need to be built")
    option(JEMALLOC_FOUND OFF)
endif()

# Add hare-config
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/include)

# Add hare_base's src
file(GLOB HARE_BASE_SOURCES
    "base/*.cc"
    "base/log/*.cc"
    "base/thread/*.cc"
    "base/time/*.cc"
    "base/util/*.cc"
)

# Add the source code to the project's executable。
add_library(hare_base ${HARE__LIBRARY_TYPE} ${HARE_BASE_SOURCES})
target_include_directories(hare_base PUBLIC ${CMAKE_SOURCE_DIR}/include)

if(JEMALLOC_FOUND)
    target_link_libraries(hare_base PkgConfig::Jemalloc)
endif()

if(HARE__BUILD_TESTS)
    add_subdirectory(base/tests)
endif(HARE__BUILD_TESTS)


if(EXISTS "${CMAKE_SOURCE_DIR}/lib/openssl/" AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/openssl/")
    message(STATUS "OpenSSL is built")
    set(ENV{PKG_CONFIG_PATH} ${CMAKE_SOURCE_DIR}/lib/openssl/lib64/pkgconfig:$ENV{PKG_CONFIG_PATH})
    find_package(OpenSSL REQUIRED SSL Crypto)
    include_directories(${OPENSSL_INCLUDE_DIR})
else()
    message(STATUS "OpenSSL need to be built")
endif()

if(OPENSSL_FOUND)
    message(STATUS "OPENSSL_VERSION = ${OPENSSL_VERSION}")
    message(STATUS "OPENSSL_INCLUDE_DIR = ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OPENSSL_LIBRARIES = ${OPENSSL_LIBRARIES}")
endif(OPENSSL_FOUND)

# Add hare_net's src
file(GLOB HARE_NET_SOURCES
    "net/*.cc"
    "net/core/*.cc"
    "net/core/reactor/*.cc"
)

# Add the source code to the project's executable。
add_library(hare_net ${HARE__LIBRARY_TYPE} ${HARE_NET_SOURCES})
target_include_directories(hare_net PUBLIC ${CMAKE_SOURCE_DIR}/include)

if(JEMALLOC_FOUND)
    target_link_libraries(hare_net hare_base PkgConfig::Jemalloc ${OPENSSL_LIBRARIES})
else()
    target_link_libraries(hare_net hare_base ${OPENSSL_LIBRARIES})
endif()

if(HARE__BUILD_TESTS)
    add_subdirectory(net/tests)
endif(HARE__BUILD_TESTS)

# Add hare_core's src
file(GLOB HARE_CORE_SOURCES
    "core/*.cc"
)

file(GLOB HARE_CORE_RTMP_SOURCES
    "core/rtmp/*.cc"
)

# Add the source code to the project's executable。
add_library(hare_core   ${HARE__LIBRARY_TYPE}
                        ${HARE_CORE_SOURCES}
                        ${HARE_CORE_RTMP_SOURCES})
target_include_directories(hare_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

if(JEMALLOC_FOUND)
    target_link_libraries(hare_core hare_net hare_base PkgConfig::Jemalloc ${OPENSSL_LIBRARIES})
else()
    target_link_libraries(hare_core hare_net hare_base ${OPENSSL_LIBRARIES})
endif()

# Add hare_main's src
file(GLOB_RECURSE HARE_MAIN_SOURCES
    "main/*.cc"
)

# Add the source code to the project's executable。
add_executable(hare_main ${HARE_MAIN_SOURCES})
if(JEMALLOC_FOUND)
    target_link_libraries(hare_main hare_core hare_net hare_base PkgConfig::Jemalloc ${OPENSSL_LIBRARIES})
else()
    target_link_libraries(hare_main hare_core hare_net hare_base ${OPENSSL_LIBRARIES})
endif()