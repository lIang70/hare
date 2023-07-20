/**
 * @file hare/base/fwd.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro and functions
 *   associated with fwd.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_FWD_H_
#define _HARE_BASE_FWD_H_

#include <array>
#include <cassert>
#include <cinttypes>
#include <cstring>
#include <functional>
#include <string>
#include <memory>

#include <hare/base/util/system_check.h>

#if defined(__clang__) && !defined(__ibmxl__)
#define HARE_CLANG_VERSION (__clang_major__ * 100 + __clang_minor__)
#else
#define HARE_CLANG_VERSION 0
#endif

#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER) && !defined(__NVCOMPILER)
#define HARE_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#else
#define HARE_GCC_VERSION 0
#endif

#ifdef _MSC_VER
#define HARE_MSC_VERSION _MSC_VER
#define HARE_MSC_WARNING(...) __pragma(warning(__VA_ARGS__))
#else
#define HARE_MSC_VERSION 0
#define HARE_MSC_WARNING(...)
#endif

#ifndef HARE_INLINE
#if HARE_GCC_VERSION || HARE_CLANG_VERSION
#define HARE_INLINE inline __attribute__((always_inline))
#else
#define HARE_INLINE inline
#endif
#endif

#if defined(H_OS_WIN32)
#define HARE_CLASS_API HARE_MSC_WARNING(suppress : 4275)
#ifdef HARE_STATIC
#define HARE_API __declspec(dllimport)
#elif defined(HARE_SHARED)
#define HARE_API __declspec(dllexport)
#endif
#else
#define HARE_CLASS_API
#if defined(HARE_STATIC) || defined(HARE_SHARED)
#if defined(__GNUC__) || defined(__clang__)
#define HARE_API __attribute__((visibility("default")))
#endif
#endif
#endif
#ifndef HARE_API
#define HARE_API
#endif

#define HARE_ALIGNAS(n) alignas(n)

#define HARE_SMALL_FIXED_SIZE (32)
#define HARE_SMALL_BUFFER (4 * 1024)
#define HARE_LARGE_BUFFER (1024 * HARE_SMALL_BUFFER)

namespace hare {

#ifdef H_OS_WIN32
using util_socket_t = intptr_t;
#else
using util_socket_t = int;
#endif

template <typename T>
using ptr = std::shared_ptr<T>;
template <typename T>
using wptr = std::weak_ptr<T>;
template <typename T>
using uptr = std::unique_ptr<T>;
using task = std::function<void()>;
#if defined(H_OS_WIN32) && defined(HARE_WCHAR_FILENAME)
using filename_t = std::wstring;
HARE_INLINE
std::string filename_to_str(const filename_t& filename)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> c;
    return c.to_bytes(filename);
}

#define HARE_FILENAME_T_INNER(s) L##s
#define HARE_FILENAME_T(s) HARE_FILENAME_T_INNER(s)
#else
using filename_t = std::string;
HARE_INLINE
std::string filename_to_str(const filename_t& filename)
{
    return filename;
}

#define HARE_FILENAME_T(s) s
#endif

// Suppresses "unused variable" warnings with the method described in
// https://herbsutter.com/2009/10/18/mailbag-shutting-up-compiler-warnings/.
// (void)var does not work on many Intel compilers.
template <typename... T> HARE_INLINE void ignore_unused(const T&...) { }

namespace detail {

    HARE_CLASS_API
        struct HARE_API impl { virtual ~impl() = default; };


#define HARE_IMPL_DEFAULT(Class, ...)                   \
    struct Class##_impl : public hare::detail::impl {   \
        __VA_ARGS__                                     \
        ~Class##_impl() override = default;             \
    };                                                  \
    HARE_INLINE auto d_ptr(hare::detail::impl* _impl)   \
    -> Class##_impl* { return down_cast<Class##_impl*>(_impl); }

#define HARE_IMPL(Class, ...)                           \
    struct Class##_impl : public hare::detail::impl {   \
        __VA_ARGS__                                     \
        ~Class##_impl() override;                       \
    };                                                  \
    HARE_INLINE auto d_ptr(hare::detail::impl* _impl)   \
        -> Class##_impl* { return down_cast<Class##_impl*>(_impl); }


    template <typename Int>
    auto to_unsigned(Int value) ->
        typename std::make_unsigned<Int>::type
    {
        assert(std::is_unsigned<Int>::value || value >= 0);
        return static_cast<typename std::make_unsigned<Int>::type>(value);
    }

    template <typename T, typename Size>
    auto fill_n(T* out, Size count, char value) -> T*
    {
        std::memset(out, value, to_unsigned(count));
        return out + count;
    }

} // namesapce detail

// Taken from google-protobuf stubs/common.h
//
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda) and others
//
// Contains basic types and utilities used by the rest of the library.

//
// Use implicit_cast as a safe version of static_cast or const_cast
// for upcasting in the type hierarchy (i.e. casting a pointer to Foo
// to a pointer to SuperclassOfFoo or casting a pointer to Foo to
// a const pointer to Foo).
// When you use implicit_cast, the compiler checks that the cast is safe.
// Such explicit implicit_casts are necessary in surprisingly many
// situations where C++ demands an exact type match instead of an
// argument type convertable to a target type.
//
// The From type can be inferred, so the preferred syntax for using
// implicit_cast is the same as for static_cast etc.:
//
//   implicit_cast<ToType>(expr)
//
// implicit_cast would have been part of the C++ standard library,
// but the proposal was submitted too late.  It will probably make
// its way into the language in the future.
template <typename To, typename From>
HARE_INLINE
auto implicit_cast(From const& _from) -> To
{
    return _from;
}

template<typename To, typename From>
HARE_INLINE
auto down_cast(From* f) -> To
{
    // Ensures that To is a sub-type of From *.  This test is here only
    // for compile-time type checking, and has no overhead in an
    // optimized build at run-time, as it will be optimized away
    // completely.
    if (false) {
        implicit_cast<From*, To>(0);
    }

#if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
    assert(f == NULL || dynamic_cast<To>(f) != NULL);  // RTTI: debug mode only!
#endif
    return static_cast<To>(f);
}

enum : std::uint8_t { TRACE_MSG, ERROR_MSG };
using log_handler = std::function<void(std::uint8_t, std::string)>;

HARE_API void register_msg_handler(log_handler handle);

} // namespace hare

#endif // _HARE_BASE_FWD_H_
