#ifndef _HARE_BASE_SYSTEM_CHECK_H_
#define _HARE_BASE_SYSTEM_CHECK_H_

/**
 * The operating system, must be one of: (H_OS_x)
 *
 *   WIN32    - Win32 (Windows 2000/XP/Vista/7 and Windows Server 2003/2008)
 *   WINRT    - WinRT (Windows Runtime)
 *   LINUX    - Linux
 *   UNIX     - Any UNIX BSD/SYSV system
 */

// Windows
#if !defined(SAG_COM) && (!defined(WINAPI_FAMILY) || WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) && (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
#define H_OS_WIN32
#define H_OS_WIN64
#elif !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#if defined(WINAPI_FAMILY)
#ifndef WINAPI_FAMILY_PC_APP
#define WINAPI_FAMILY_PC_APP WINAPI_FAMILY_APP
#endif
#if defined(WINAPI_FAMILY_PHONE_APP) && WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
#define H_OS_WINRT
#elif WINAPI_FAMILY == WINAPI_FAMILY_PC_APP
#define H_OS_WINRT
#else
#define H_OS_WIN32
#endif
#else
#define H_OS_WIN32
#endif
// LINUX
#elif defined(__linux__) || defined(__linux)
#define H_OS_LINUX
#else
#error "not support this OS"
#endif

#if defined(H_OS_WIN)
#undef H_OS_UNIX
#elif !defined(H_OS_UNIX)
#define H_OS_UNIX
#endif

#endif // !_HARE_BASE_SYSTEM_CHECK_H_