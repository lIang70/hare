#include <hare/net/tcp_session.h>

#include "hare/net/socket_op.h"
#include "hare/base/io/reactor.h"
#include <hare/base/io/cycle.h>
#include <hare/base/logging.h>

namespace hare {
namespace net {

    tcp_session::~tcp_session() = default;

    auto tcp_session::append(io::buffer& _buffer) -> bool
    {
        /// FIXME:
        if (state() == STATE_CONNECTED) {
            size_t out_buffer_size { 0 };
            {
                std::unique_lock<std::mutex> lock(write_mutex_);
                out_buffer_size = out_buffer().size();
                out_buffer().append(_buffer);
            }
            if (out_buffer_size == 0) {
                owner_cycle()->queue_in_cycle(std::bind([=](const wptr<session>& session) {
                    auto tcp = session.lock();
                    if (tcp) {
                        event()->enable_write();
                        handle_write();
                    }
                },
                    std::static_pointer_cast<tcp_session>(shared_from_this())));
            } else if (out_buffer_size > high_water_mark()) {
                owner_cycle()->queue_in_cycle(std::bind([=](const wptr<tcp_session>& session) {
                    auto tcp = session.lock();
                    if (tcp) {
                        high_water_(shared_from_this());
                    }
                },
                    std::static_pointer_cast<tcp_session>(shared_from_this())));
            }
            return true;
        }
        return false;
    }

    auto tcp_session::send(void* _bytes, size_t _length) -> bool
    {
        /// FIXME:
        if (state() == STATE_CONNECTED) {
            size_t out_buffer_size { 0 };
            {
                std::unique_lock<std::mutex> lock(write_mutex_);
                out_buffer_size = out_buffer().size();
                out_buffer().add(_bytes, _length);
            }
            if (out_buffer_size == 0) {
                owner_cycle()->queue_in_cycle(std::bind([=](const wptr<session>& session) {
                    auto tcp = session.lock();
                    if (tcp) {
                        event()->enable_write();
                        handle_write();
                    }
                },
                    std::static_pointer_cast<tcp_session>(shared_from_this())));
            } else if (out_buffer_size > high_water_mark()) {
                owner_cycle()->queue_in_cycle(std::bind([=](const wptr<tcp_session>& session) {
                    auto tcp = session.lock();
                    if (tcp) {
                        high_water_(shared_from_this());
                    }
                },
                    std::static_pointer_cast<tcp_session>(shared_from_this())));
            }
            return true;
        }
        return false;
    }

    auto tcp_session::set_tcp_no_delay(bool _on) -> error
    {
        return socket().set_tcp_no_delay(_on);
    }

    tcp_session::tcp_session(io::cycle* _cycle,
        host_address _local_addr,
        std::string _name, int8_t _family, util_socket_t _fd,
        host_address _peer_addr)
        : session(_cycle, TYPE_TCP,
            std::move(_local_addr),
            std::move(_name), _family, _fd,
            std::move(_peer_addr))
    {
        socket().set_keep_alive(true);
    }

    void tcp_session::handle_read(const timestamp& _time)
    {
        auto read_n = in_buffer().read(fd(), -1);
        if (read_n == 0) {
            handle_close();
        } else if (read_n > 0) {
            read_(shared_from_this(), in_buffer(), _time);
        } else {
            handle_error();
        }
    }

    void tcp_session::handle_write()
    {
        if (event()->writing()) {
            std::unique_lock<std::mutex> lock(write_mutex_);
            auto write_n = out_buffer().write(fd(), -1);
            if (write_n >= 0) {
                if (out_buffer().size() == 0) {
                    event()->disable_write();
                    owner_cycle()->queue_in_cycle(std::bind([=] (const wptr<tcp_session>& session) {
                        auto tcp = session.lock();
                        if (tcp) {
                            if (write_) {
                                write_(tcp);
                            } else {
                                SYS_ERROR() << "not set write_callback to tcp session[" << name() << "].";
                            }
                        }
                    },
                        std::static_pointer_cast<tcp_session>(shared_from_this())));
                }
                if (state() == STATE_DISCONNECTING) {
                    shutdown();
                }
            } else {
                SYS_ERROR() << "an error occurred while writing the socket, detail: " << socket_op::get_socket_error_info(fd());
            }
        } else {
            LOG_TRACE() << "tcp session[fd: " << fd() << ", name: " << name() << "] is down, no more writing";
        }
    }

} // namespace net
} // namespace hare
