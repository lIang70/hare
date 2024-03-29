// hare-config.h
//
// This file was generated by cmake when the makefiles were generated.
//
// DO NOT EDIT THIS FILE.
//
// Do not rely on macros in this file existing in later versions.
//
#ifndef _HARE_CONFIG_H_
#define _HARE_CONFIG_H_

/* Numeric representation of the version */
#define HARE__VERSION_MAJOR @HARE_VERSION_MAJOR@
#define HARE__VERSION_MINOR @HARE_VERSION_MINOR@
#define HARE__VERSION_PATCH @HARE_VERSION_PATCH@

/* Version number of package */
#define HARE__VERSION "@HARE_PACKAGE_VERSION@"

/* Name of package */
#define HARE__PACKAGE "hare"

/* Define to 1 if you have the `accept4' function. */
#cmakedefine HARE__HAVE_ACCEPT4 1

/* Define to 1 if you have the `arc4random' function. */
#cmakedefine HARE__HAVE_ARC4RANDOM 1

/* Define to 1 if you have the `arc4random_buf' function. */
#cmakedefine HARE__HAVE_ARC4RANDOM_BUF 1

/* Define to 1 if you have the `arc4random_addrandom' function. */
#cmakedefine HARE__HAVE_ARC4RANDOM_ADDRANDOM 1

/* Define if clock_gettime is available in libc */
#cmakedefine HARE__DNS_USE_CPU_CLOCK_FOR_ID 1

/* Define is no secure id variant is available */
#cmakedefine HARE__DNS_USE_GETTIMEOFDAY_FOR_ID 1
#cmakedefine HARE__DNS_USE_FTIME_FOR_ID 1

/* Define to 1 if you have the <arpa/inet.h> header file. */
#cmakedefine HARE__HAVE_ARPA_INET_H 1

/* Define to 1 if you have the `clock_gettime' function. */
#cmakedefine HARE__HAVE_CLOCK_GETTIME 1

/* Define to 1 if you have the declaration of `CTL_KERN'. */
#define HARE__HAVE_DECL_CTL_KERN @HARE__HAVE_DECL_CTL_KERN@

/* Define to 1 if you have the declaration of `KERN_ARND'. */
#define HARE__HAVE_DECL_KERN_ARND @HARE__HAVE_DECL_KERN_ARND@

/* Define to 1 if you have `getrandom' function. */
#cmakedefine HARE__HAVE_GETRANDOM 1

/* Define if /dev/poll is available */
#cmakedefine HARE__HAVE_DEVPOLL 1

/* Define to 1 if you have the <netdb.h> header file. */
#cmakedefine HARE__HAVE_NETDB_H 1

/* Define to 1 if fd_mask type is defined */
#cmakedefine HARE__HAVE_FD_MASK 1

/* Define to 1 if the <sys/queue.h> header file defines TAILQ_FOREACH. */
#cmakedefine HARE__HAVE_TAILQFOREACH 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HARE__HAVE_DLFCN_H 1

/* Define if your system supports the epoll system calls */
#cmakedefine HARE__HAVE_EPOLL 1

/* Define to 1 if you have the `epoll_create1' function. */
#cmakedefine HARE__HAVE_EPOLL_CREATE1 1

/* Define to 1 if you have the `epoll_pwait2' function. */
#cmakedefine HARE__HAVE_EPOLL_PWAIT2 1

/* Define to 1 if you have the `epoll_ctl' function. */
#cmakedefine HARE__HAVE_EPOLL_CTL 1

/* Define if your system supports the wepoll module */
#cmakedefine HARE__HAVE_WEPOLL 1

/* Define to 1 if you have the `eventfd' function. */
#cmakedefine HARE__HAVE_EVENTFD 1

/* Define if your system supports event ports */
#cmakedefine HARE__HAVE_HARE_PORTS 1

/* Define to 1 if you have the `fcntl' function. */
#cmakedefine HARE__HAVE_FCNTL 1

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HARE__HAVE_FCNTL_H 1

/* Define to 1 if you have the `getaddrinfo' function. */
#cmakedefine HARE__HAVE_GETADDRINFO 1

/* Define to 1 if you have the `getegid' function. */
#cmakedefine HARE__HAVE_GETEGID 1

/* Define to 1 if you have the `geteuid' function. */
#cmakedefine HARE__HAVE_GETEUID 1

/* TODO: Check for different gethostname argument counts. CheckPrototypeDefinition.cmake can be used. */
/* Define this if you have any gethostbyname_r() */
#cmakedefine HARE__HAVE_GETHOSTBYNAME_R 1

/* Define this if gethostbyname_r takes 3 arguments */
#cmakedefine HARE__HAVE_GETHOSTBYNAME_R_3_ARG 1

/* Define this if gethostbyname_r takes 5 arguments */
#cmakedefine HARE__HAVE_GETHOSTBYNAME_R_5_ARG 1

/* Define this if gethostbyname_r takes 6 arguments */
#cmakedefine HARE__HAVE_GETHOSTBYNAME_R_6_ARG 1

/* Define to 1 if you have the `getifaddrs' function. */
#cmakedefine HARE__HAVE_GETIFADDRS 1

/* Define to 1 if you have the `getnameinfo' function. */
#cmakedefine HARE__HAVE_GETNAMEINFO 1

/* Define to 1 if you have the `getprotobynumber' function. */
#cmakedefine HARE__HAVE_GETPROTOBYNUMBER 1

/* Define to 1 if you have the `getservbyname' function. */
#cmakedefine HARE__HAVE_GETSERVBYNAME 1

/* Define to 1 if you have the `gettimeofday' function. */
#cmakedefine HARE__HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the <ifaddrs.h> header file. */
#cmakedefine HARE__HAVE_IFADDRS_H 1

/* Define to 1 if you have the `inet_ntop' function. */
#cmakedefine HARE__HAVE_INET_NTOP 1

/* Define to 1 if you have the `inet_pton' function. */
#cmakedefine HARE__HAVE_INET_PTON 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HARE__HAVE_INTTYPES_H 1

/* Define to 1 if you have the `issetugid' function. */
#cmakedefine HARE__HAVE_ISSETUGID 1

/* Define to 1 if you have the `kqueue' function. */
#cmakedefine HARE__HAVE_KQUEUE 1

/* Define if the system has zlib */
#cmakedefine HARE__HAVE_LIBZ 1

/* Define to 1 if you have the `mach_absolute_time' function. */
#cmakedefine HARE__HAVE_MACH_ABSOLUTE_TIME 1

/* Define to 1 if you have the <mach/mach_time.h> header file. */
#cmakedefine HARE__HAVE_MACH_MACH_TIME_H 1

/* Define to 1 if you have the <mach/mach.h> header file. */
#cmakedefine HARE__HAVE_MACH_MACH_H 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HARE__HAVE_MEMORY_H 1

/* Define to 1 if you have the `mmap' function. */
#cmakedefine HARE__HAVE_MMAP 1

/* Define to 1 if you have the `mmap64' function. */
#cmakedefine HARE__HAVE_MMAP64 1

/* Define to 1 if you have the `nanosleep' function. */
#cmakedefine HARE__HAVE_NANOSLEEP 1

/* Define to 1 if you have the `usleep' function. */
#cmakedefine HARE__HAVE_USLEEP 1

/* Define to 1 if you have the <netinet/in6.h> header file. */
#cmakedefine HARE__HAVE_NETINET_IN6_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#cmakedefine HARE__HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <netinet/tcp.h> header file. */
#cmakedefine HARE__HAVE_NETINET_TCP_H 1

/* Define to 1 if you have the <sys/un.h> header file. */
#cmakedefine HARE__HAVE_SYS_UN_H 1

/* Define to 1 if you have the <afunix.h> header file. */
#cmakedefine HARE__HAVE_AFUNIX_H 1

/* Define if the system has openssl */
#cmakedefine HARE__HAVE_OPENSSL 1

/* Define if the system has mbedtls */
#cmakedefine HARE__HAVE_MBEDTLS 1

/* Define to 1 if you have the `pipe' function. */
#cmakedefine HARE__HAVE_PIPE 1

/* Define to 1 if you have the `pipe2' function. */
#cmakedefine HARE__HAVE_PIPE2 1

/* Define to 1 if you have the `poll' function. */
#cmakedefine HARE__HAVE_POLL 1

/* Define to 1 if you have the <poll.h> header file. */
#cmakedefine HARE__HAVE_POLL_H 1

/* Define to 1 if you have the `port_create' function. */
#cmakedefine HARE__HAVE_PORT_CREATE 1

/* Define to 1 if you have the <port.h> header file. */
#cmakedefine HARE__HAVE_PORT_H 1

/* Define if we have pthreads on this system */
#cmakedefine HARE__HAVE_PTHREADS 1

/* Define to 1 if you have the `pthread_mutexattr_setprotocol' function. */
#cmakedefine HARE__HAVE_PTHREAD_MUTEXATTR_SETPROTOCOL 1

/* Define to 1 if you have the `putenv' function. */
#cmakedefine HARE__HAVE_PUTENV 1

/* Define to 1 if the system has the type `sa_family_t'. */
#cmakedefine HARE__HAVE_SA_FAMILY_T 1

/* Define to 1 if you have the `select' function. */
#cmakedefine HARE__HAVE_SELECT 1

/* Define to 1 if you have the `setenv' function. */
#cmakedefine HARE__HAVE_SETENV 1

/* Define if F_SETFD is defined in <fcntl.h> */
#cmakedefine HARE__HAVE_SETFD 1

/* Define to 1 if you have the `setrlimit' function. */
#cmakedefine HARE__HAVE_SETRLIMIT 1

/* Define to 1 if you have the `sendfile' function. */
#cmakedefine HARE__HAVE_SENDFILE 1

/* Define to 1 if you have the `sigaction' function. */
#cmakedefine HARE__HAVE_SIGACTION 1

/* Define to 1 if you have the `signal' function. */
#cmakedefine HARE__HAVE_SIGNAL 1

/* Define to 1 if you have the `strsignal' function. */
#cmakedefine HARE__HAVE_STRSIGNAL 1

/* Define to 1 if you have the <stdarg.h> header file. */
#cmakedefine HARE__HAVE_STDARG_H 1

/* Define to 1 if you have the <stddef.h> header file. */
#cmakedefine HARE__HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HARE__HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HARE__HAVE_STDLIB_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HARE__HAVE_STRING_H 1

/* Define to 1 if you have the `strlcpy' function. */
#cmakedefine HARE__HAVE_STRLCPY 1

/* Define to 1 if you have the `strsep' function. */
#cmakedefine HARE__HAVE_STRSEP 1

/* Define to 1 if you have the `strtok_r' function. */
#cmakedefine HARE__HAVE_STRTOK_R 1

/* Define to 1 if you have the `strtoll' function. */
#cmakedefine HARE__HAVE_STRTOLL 1

/* Define to 1 if you have the `_gmtime64_s' function. */
#cmakedefine HARE__HAVE__GMTIME64_S 1

/* Define to 1 if you have the `_gmtime64' function. */
#cmakedefine HARE__HAVE__GMTIME64 1

/* Define to 1 if the system has the type `struct addrinfo'. */
#cmakedefine HARE__HAVE_STRUCT_ADDRINFO 1

/* Define to 1 if the system has the type `struct in6_addr'. */
#cmakedefine HARE__HAVE_STRUCT_IN6_ADDR 1

/* Define to 1 if `s6_addr16' is member of `struct in6_addr'. */
#cmakedefine HARE__HAVE_STRUCT_IN6_ADDR_S6_ADDR16 1

/* Define to 1 if `s6_addr32' is member of `struct in6_addr'. */
#cmakedefine HARE__HAVE_STRUCT_IN6_ADDR_S6_ADDR32 1

/* Define to 1 if the system has the type `struct sockaddr_in6'. */
#cmakedefine HARE__HAVE_STRUCT_SOCKADDR_IN6 1

/* Define to 1 if `sin6_len' is member of `struct sockaddr_in6'. */
#cmakedefine HARE__HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN 1

/* Define to 1 if `sin_len' is member of `struct sockaddr_in'. */
#cmakedefine HARE__HAVE_STRUCT_SOCKADDR_IN_SIN_LEN 1

/* Define to 1 if the system has the type `struct sockaddr_un'. */
#cmakedefine HARE__HAVE_STRUCT_SOCKADDR_UN 1

/* Define to 1 if the system has the type `struct sockaddr_storage'. */
#cmakedefine HARE__HAVE_STRUCT_SOCKADDR_STORAGE 1

/* Define to 1 if `ss_family' is a member of `struct sockaddr_storage'. */
#cmakedefine HARE__HAVE_STRUCT_SOCKADDR_STORAGE_SS_FAMILY 1

/* Define to 1 if `__ss_family' is a member of `struct sockaddr_storage'. */
#cmakedefine HARE__HAVE_STRUCT_SOCKADDR_STORAGE___SS_FAMILY 1

/* Define to 1 if the system has the type `struct linger'. */
#cmakedefine HARE__HAVE_STRUCT_LINGER 1

/* Define to 1 if you have the `sysctl' function. */
#cmakedefine HARE__HAVE_SYSCTL 1

/* Define to 1 if you have the <sys/epoll.h> header file. */
#cmakedefine HARE__HAVE_SYS_EPOLL_H 1

/* Define to 1 if you have the <sys/eventfd.h> header file. */
#cmakedefine HARE__HAVE_SYS_EVENTFD_H 1

/* Define to 1 if you have the <sys/event.h> header file. */
#cmakedefine HARE__HAVE_SYS_EVENT_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#cmakedefine HARE__HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/mman.h> header file. */
#cmakedefine HARE__HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#cmakedefine HARE__HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/queue.h> header file. */
#cmakedefine HARE__HAVE_SYS_QUEUE_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
#cmakedefine HARE__HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#cmakedefine HARE__HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/sendfile.h> header file. */
#cmakedefine HARE__HAVE_SYS_SENDFILE_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#cmakedefine HARE__HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HARE__HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/random.h> header file. */
#cmakedefine HARE__HAVE_SYS_RANDOM_H 1

/* Define to 1 if you have the <sys/sysctl.h> header file. */
#cmakedefine HARE__HAVE_SYS_SYSCTL_H 1

/* Define to 1 if you have the <sys/timerfd.h> header file. */
#cmakedefine HARE__HAVE_SYS_TIMERFD_H 1

/* Define to 1 if you have the <sys/signalfd.h> header file. */
#cmakedefine HARE__HAVE_SYS_SIGNALFD_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#cmakedefine HARE__HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HARE__HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/uio.h> header file. */
#cmakedefine HARE__HAVE_SYS_UIO_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#cmakedefine HARE__HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <errno.h> header file. */
#cmakedefine HARE__HAVE_ERRNO_H 1

/* Define if TAILQ_FOREACH is defined in <sys/queue.h> */
#cmakedefine HARE__HAVE_TAILQFOREACH 1

/* Define if timeradd is defined in <sys/time.h> */
#cmakedefine HARE__HAVE_TIMERADD 1

/* Define if timerclear is defined in <sys/time.h> */
#cmakedefine HARE__HAVE_TIMERCLEAR 1

/* Define if timercmp is defined in <sys/time.h> */
#cmakedefine HARE__HAVE_TIMERCMP 1


/* Define to 1 if you have the `timerfd_create' function. */
#cmakedefine HARE__HAVE_TIMERFD_CREATE 1

/* Define if timerisset is defined in <sys/time.h> */
#cmakedefine HARE__HAVE_TIMERISSET 1

/* Define to 1 if the system has the type `uint8_t'. */
#cmakedefine HARE__HAVE_UINT8_T 1

/* Define to 1 if the system has the type `uint16_t'. */
#cmakedefine HARE__HAVE_UINT16_T 1

/* Define to 1 if the system has the type `uint32_t'. */
#cmakedefine HARE__HAVE_UINT32_T 1

/* Define to 1 if the system has the type `uint64_t'. */
#cmakedefine HARE__HAVE_UINT64_T 1

/* Define to 1 if the system has the type `uintptr_t'. */
#cmakedefine HARE__HAVE_UINTPTR_T 1

/* Define to 1 if you have the `umask' function. */
#cmakedefine HARE__HAVE_UMASK 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HARE__HAVE_UNISTD_H 1

/* Define to 1 if you have the `unsetenv' function. */
#cmakedefine HARE__HAVE_UNSETENV 1

/* Define if kqueue works correctly with pipes */
#cmakedefine HARE__HAVE_WORKING_KQUEUE 1

#ifdef __USE_UNUSED_DEFINITIONS__
/* Define to necessary symbol if this constant uses a non-standard name on your system. */
/* XXX: Hello, this isn't even used, nor is it defined anywhere... - Ellzey */
#define HARE__PTHREAD_CREATE_JOINABLE ${HARE__PTHREAD_CREATE_JOINABLE}
#endif

/* The size of `pthread_t', as computed by sizeof. */
#define HARE__SIZEOF_PTHREAD_T @HARE__SIZEOF_PTHREAD_T@

/* The size of a `int', as computed by sizeof. */
#define HARE__SIZEOF_INT @HARE__SIZEOF_INT@

/* The size of a `long', as computed by sizeof. */
#define HARE__SIZEOF_LONG @HARE__SIZEOF_LONG@

/* The size of a `long long', as computed by sizeof. */
#define HARE__SIZEOF_LONG_LONG @HARE__SIZEOF_LONG_LONG@

/* The size of `off_t', as computed by sizeof. */
#define HARE__SIZEOF_OFF_T @HARE__SIZEOF_OFF_T@

#define HARE__SIZEOF_SSIZE_T @HARE__SIZEOF_SSIZE_T@


/* The size of a `short', as computed by sizeof. */
#define HARE__SIZEOF_SHORT @HARE__SIZEOF_SHORT@

/* The size of `size_t', as computed by sizeof. */
#define HARE__SIZEOF_SIZE_T @HARE__SIZEOF_SIZE_T@

/* The size of `socklen_t', as computed by sizeof. */
#define HARE__SIZEOF_SOCKLEN_T @HARE__SIZEOF_SOCKLEN_T@

/* The size of 'void *', as computer by sizeof */
#define HARE__SIZEOF_VOID_P @HARE__SIZEOF_VOID_P@

/* The size of 'time_t', as computer by sizeof */
#define HARE__SIZEOF_TIME_T @HARE__SIZEOF_TIME_T@

/* Define to `__inline__' or `__inline' if that's what the C compilercallsit, or to nothing if'inline' is not supported under any name. */
#ifndef __cplusplus
/* why not c++?
*
*  and are we really expected to use HARE__inline everywhere,
*  shouldn't we just do:
*     ifdef HARE__inline
*     define inline HARE__inline
*
* - Ellzey
*/

#define HARE__inline @HARE__inline@
#endif

#cmakedefine HARE__HAVE___func__ 1
#cmakedefine HARE__HAVE___FUNCTION__ 1

/* Define to `unsigned' if <sys/types.h> does not define. */
#define HARE__size_t @HARE__size_t@

/* Define to unsigned int if you dont have it */
#define HARE__socklen_t @HARE__socklen_t@

/* Define to `int' if <sys/types.h> does not define. */
# define HARE__ssize_t @HARE__ssize_t@

# endif /* \_HARE_CONFIG_H_ */
