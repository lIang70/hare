#ifndef _HARE_NET_INTERNAL_H_
#define _HARE_NET_INTERNAL_H_

#include <cinttypes>

#ifdef HARE__HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif


namespace hare {
namespace net {

#ifdef HARE__HAVE_SYS_UIO_H
    using IoVec = iovec;
/* Internal use -- defined only if we are using the native struct iovec */
#define HARE_IOVEC_IS_NATIVE
#else
    struct IoVec {
        /** The start of the extent of memory. */
        void* iov_base;
        /** The length of the extent of memory. */
        std::size_t iov_len;
    };
#endif

} // namespace net
} // namespace hare

#endif // !_HARE_NET_INTERNAL_H_
