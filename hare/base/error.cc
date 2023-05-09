#include <hare/base/error.h>

#include <array>
#include <cerrno>

namespace hare {

namespace detail {

    static std::array<const char*, HARE_ERRORS_NBR> error_description {
        "Illegal error occurred, please contact the developer.",    // HARE_ERROR_ILLEGAL
        "Success.",                                                 // HARE_ERROR_SUCCESS
        
        // Base
        "Cannot set name of thread.",                               // HARE_ERROR_SET_THREAD_NAME

        // Net
        "Cannot get local address.",                                // HARE_ERROR_GET_LOCAL_ADDRESS
        "The family of address is [AF_INET6/AF_INET] .",            // HARE_ERROR_WRONG_FAMILY
        "Failed to bind socket.",                                   // HARE_ERROR_SOCKET_BIND
        "Failed to listen at specified port  .",                    // HARE_ERROR_SOCKET_LISTEN
        "Failed to close socket.",                                  // HARE_ERROR_SOCKET_CLOSED
        "Failed to connect socket.",                                // HARE_ERROR_SOCKET_CONNECT
        "Failed to shutdown write of socket.",                      // HARE_ERROR_SOCKET_SHUTDOWN_WRITE
        "Failed to get addr from ip/port.",                         // HARE_ERROR_SOCKET_FROM_IP
        "Failed to set no-delay to tcp socket.",                    // HARE_ERROR_SOCKET_TCP_NO_DELAY
        "Failed to set reuse address to socket.",                   // HARE_ERROR_SOCKET_REUSE_ADDR
        "Failed to set reuse port to socket.",                      // HARE_ERROR_SOCKET_REUSE_PORT
        "Failed to set keep alive to socket.",                      // HARE_ERROR_SOCKET_KEEP_ALIVE
        "Failed to shutdown, because socket is writing.",           // HARE_ERROR_SOCKET_WRITING
        "Failed to active acceptor.",                               // HARE_ERROR_ACCEPTOR_ACTIVED
        "Session already disconnected.",                            // HARE_ERROR_SESSION_ALREADY_DISCONNECT
        "Failed to get pair socket.",                               // HARE_ERROR_GET_SOCKET_PAIR
        "Failed to init io pool.",                                  // HARE_ERROR_INIT_IO_POOL
    };

} // namespace detail

error::error(int32_t error_code)
    : error_code_(error_code)
    , system_code_(errno)
{
}

auto error::description() const -> const char*
{
    return detail::error_description[error_code_];
}

} // namespace hare