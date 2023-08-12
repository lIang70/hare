#include <hare/net/error.h>

#include <array>
#include <cerrno>

namespace hare {
namespace net {

    namespace detail {

        static std::array<const char*, ERRORS_NBR + 1> error_description {
            "Illegal error occurred, please contact the developer.", // ERROR_ILLEGAL
            "Success.", // ERROR_SUCCESS

            // Net
            "Cannot get local address.",                                // ERROR_GET_LOCAL_ADDRESS
            "The family of address is [AF_INET6/AF_INET] .",            // ERROR_WRONG_FAMILY
            "Failed to bind socket.",                                   // ERROR_SOCKET_BIND
            "Failed to listen at specified port  .",                    // ERROR_SOCKET_LISTEN
            "Failed to close socket.",                                  // ERROR_SOCKET_CLOSED
            "Failed to connect socket.",                                // ERROR_SOCKET_CONNECT
            "Failed to shutdown write of socket.",                      // ERROR_SOCKET_SHUTDOWN_WRITE
            "Failed to get addr from ip/port.",                         // ERROR_SOCKET_FROM_IP
            "Failed to set no-delay to tcp socket.",                    // ERROR_SOCKET_TCP_NO_DELAY
            "Failed to set reuse address to socket.",                   // ERROR_SOCKET_REUSE_ADDR
            "Failed to set reuse port to socket.",                      // ERROR_SOCKET_REUSE_PORT
            "Failed to set keep alive to socket.",                      // ERROR_SOCKET_KEEP_ALIVE
            "Failed to shutdown, because socket is writing.",           // ERROR_SOCKET_WRITING
            "Failed to active acceptor.",                               // ERROR_ACCEPTOR_ACTIVED
            "Session already disconnected.",                            // ERROR_SESSION_ALREADY_DISCONNECT
            "Failed to get pair socket.",                               // ERROR_GET_SOCKET_PAIR
            "Failed to init io pool.",                                  // ERROR_INIT_IO_POOL
        };

    } // namespace detail

    Error::Error(std::int32_t error_code)
        : error_code_(error_code)
        , system_code_(errno)
    {
    }

    auto Error::Description() const -> const char*
    {
        return detail::error_description[error_code_];
    }

} // namespace net
} // namespace hare