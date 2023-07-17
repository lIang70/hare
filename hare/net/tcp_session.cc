#include "hare/base/fwd-inl.h"
#include "hare/net/socket_op.h"
#include <hare/base/io/cycle.h>
#include <hare/net/tcp_session.h>

namespace hare {
namespace net {

    tcp_session::~tcp_session() = default;

    auto tcp_session::append(buffer& _buffer) -> bool
    {
        if (state() == STATE_CONNECTED) {
            auto tmp = std::make_shared<buffer>();
            tmp->append(_buffer);
            owner_cycle()->queue_in_cycle(std::bind([](const wptr<tcp_session>& session, buffer::ptr& buffer) {
                auto tcp = session.lock();
                if (tcp) {
                    auto out_buffer_size = tcp->out_buffer_.size();
                    tcp->out_buffer_.append(*buffer);
                    if (out_buffer_size == 0) {
                        tcp->event()->enable_write();
                        tcp->handle_write();
                    } else if (out_buffer_size > tcp->high_water_mark_) {
                        tcp->high_water_(tcp);
                    }
                }
            },
                std::static_pointer_cast<tcp_session>(shared_from_this()), std::move(tmp)));
            return true;
        }
        return false;
    }

    auto tcp_session::send(const void* _bytes, size_t _length) -> bool
    {
        if (state() == STATE_CONNECTED) {
            auto tmp = std::make_shared<buffer>();
            tmp->add(_bytes, _length);
            owner_cycle()->queue_in_cycle(std::bind([](const wptr<tcp_session>& session, buffer::ptr& buffer) {
                auto tcp = session.lock();
                if (tcp) {
                    auto out_buffer_size = tcp->out_buffer_.size();
                    tcp->out_buffer_.append(*buffer);
                    if (out_buffer_size == 0) {
                        tcp->event()->enable_write();
                        tcp->handle_write();
                    } else if (out_buffer_size > tcp->high_water_mark_) {
                        tcp->high_water_(tcp);
                    }
                }
            },
                std::static_pointer_cast<tcp_session>(shared_from_this()), std::move(tmp)));
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
        : session(CHECK_NULL(_cycle), TYPE_TCP,
            std::move(_local_addr),
            std::move(_name), _family, _fd,
            std::move(_peer_addr))
    {
        socket().set_keep_alive(true);
    }

    void tcp_session::handle_read(const timestamp& _time)
    {
        auto read_n = in_buffer_.read(fd(), -1);
        if (read_n == 0) {
            handle_close();
        } else if (read_n > 0) {
            read_(std::static_pointer_cast<tcp_session>(shared_from_this()), in_buffer_, _time);
        } else {
            handle_error();
        }
    }

    void tcp_session::handle_write()
    {
        if (event()->writing()) {
            auto write_n = out_buffer_.write(fd(), -1);
            if (write_n >= 0) {
                if (out_buffer_.size() == 0) {
                    event()->disable_write();
                    owner_cycle()->queue_in_cycle(std::bind([=](const wptr<tcp_session>& session) {
                        auto tcp = session.lock();
                        if (tcp) {
                            if (write_) {
                                write_(tcp);
                            } else {
                                MSG_ERROR("write_callback has not been set for tcp-session[{}].", name());
                            }
                        }
                    },
                        std::static_pointer_cast<tcp_session>(shared_from_this())));
                }
                if (state() == STATE_DISCONNECTING) {
                    handle_close();
                }
            } else {
                MSG_ERROR("an error occurred while writing the socket, detail: {}.", socket_op::get_socket_error_info(fd()));
            }
        } else {
            MSG_TRACE("tcp-session[fd={}, name={}] is down, no more writing.", fd(), name());
        }
    }

} // namespace net
} // namespace hare
