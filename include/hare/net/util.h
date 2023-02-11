#ifndef _HARE_NET_UTIL_H_
#define _HARE_NET_UTIL_H_

#include <hare/base/util.h>

namespace hare {

#ifdef H_OS_WIN32
using socket_t = intptr_t;
#else
using socket_t = int;
#endif

} // namespace hare

#endif // !_HARE_NET_UTIL_H_