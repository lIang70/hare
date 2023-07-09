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

#include <hare/base/util/system_check.h>

// C header
#include <cinttypes>
#include <cstring>

// C++ header
#include <array>
#include <functional>
#include <memory>

#ifdef H_OS_WIN32
#ifndef HARE_STATIC
// Windows platforms
#if HARE_EXPORTS
// From DLL side, we must export
#define HARE_API __declspec(dllexport)
#else
// From client application side, we must import
#define HARE_API __declspec(dllimport)
#endif
#else
// No specific directive needed for static build
#define HARE_API
#endif
#else
// Other platforms don't need to define anything
#define HARE_API
#endif

#define H_UNUSED(x) (void)(x)
#define H_ALIGNAS(n) alignas(n)

#define HARE_SMALL_FIXED_SIZE (32)
#define HARE_SMALL_BUFFER (4 * 1024)
#define HARE_LARGE_BUFFER (1024 * HARE_SMALL_BUFFER)

namespace hare {

#ifdef H_OS_WIN32
using util_socket_t = intptr_t;
#else
using util_socket_t = int;
#endif

template <class Ty>
using ptr = std::shared_ptr<Ty>;

template <class Ty>
using wptr = std::weak_ptr<Ty>;

template <class Ty>
using uptr = std::unique_ptr<Ty>;

using task = std::function<void()>;

inline void set_zero(void* _des, size_t _len)
{
    ::memset(_des, 0, _len);
}

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
inline auto implicit_cast(From const& _from) -> To
{
    return _from;
}

} // namespace hare

#endif // !_HARE_BASE_FWD_H_