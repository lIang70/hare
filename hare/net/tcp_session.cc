#include "hare/base/fwd-inl.h"
#include <hare/base/io/cycle.h>
#include <hare/base/io/socket_op.h>
#include <hare/net/tcp_session.h>

namespace hare {
namespace net {

    HARE_IMPL_DEFAULT(tcp_session,
        buffer out_buffer_ {};
        buffer in_buffer_ {};
        std::size_t high_water_mark_ { HARE_DEFAULT_HIGH_WATER };
        tcp_session::write_callback write_ {};
        tcp_session::high_water_callback high_water_ {};
        tcp_session::read_callback read_ {};
    )

    tcp_session::~tcp_session()
    { delete impl_; }

    void tcp_session::set_high_water_mark(size_t _hwm)
    { d_ptr(impl_)->high_water_mark_ = _hwm; }
    void tcp_session::set_read_callback(read_callback _read)
    { d_ptr(impl_)->read_ = std::move(_read); }
    void tcp_session::set_write_callback(write_callback _write)
    { d_ptr(impl_)->write_ = std::move(_write); }
    void tcp_session::set_high_water_callback(high_water_callback _high_water)
    { d_ptr(impl_)->high_water_ = std::move(_high_water); }

    auto tcp_session::append(buffer& _buffer) -> bool
    {
        if (state() == STATE_CONNECTED) {
            auto tmp = std::make_shared<buffer>();
            tmp->append(_buffer);
            owner_cycle()->queue_in_cycle(std::bind([](const wptr<tcp_session>& session, hare::ptr<buffer>& buffer) {
                auto tcp = session.lock();
                if (tcp) {
                    auto out_buffer_size = d_ptr(tcp->impl_)->out_buffer_.size();
                    d_ptr(tcp->impl_)->out_buffer_.append(*buffer);
                    if (out_buffer_size == 0) {
                        tcp->event()->enable_write();
                        tcp->handle_write();
                    } else if (out_buffer_size > d_ptr(tcp->impl_)->high_water_mark_ && d_ptr(tcp->impl_)->high_water_mark_) {
                        d_ptr(tcp->impl_)->high_water_(tcp);
                    } else if (out_buffer_size > d_ptr(tcp->impl_)->high_water_mark_) {
                        MSG_ERROR("high_water_mark_callback has not been set for tcp-session[{}].", tcp->name());
                    }
                }
            },
                std::static_pointer_cast<tcp_session>(shared_from_this()), std::move(tmp)));
            return true;
        }
        return false;
    }

    auto tcp_session::send(const void* _bytes, std::size_t _length) -> bool
    {
        if (state() == STATE_CONNECTED) {
            auto tmp = std::make_shared<buffer>();
            tmp->add(_bytes, _length);
            owner_cycle()->queue_in_cycle(std::bind([](const wptr<tcp_session>& session, hare::ptr<buffer>& buffer) {
                auto tcp = session.lock();
                if (tcp) {
                    auto out_buffer_size = d_ptr(tcp->impl_)->out_buffer_.size();
                    d_ptr(tcp->impl_)->out_buffer_.append(*buffer);
                    if (out_buffer_size == 0) {
                        tcp->event()->enable_write();
                        tcp->handle_write();
                    } else if (out_buffer_size > d_ptr(tcp->impl_)->high_water_mark_ && d_ptr(tcp->impl_)->high_water_mark_) {
                        d_ptr(tcp->impl_)->high_water_(tcp);
                    } else if (out_buffer_size > d_ptr(tcp->impl_)->high_water_mark_) {
                        MSG_ERROR("high_water_mark_callback has not been set for tcp-session[{}].", tcp->name());
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
        std::string _name, std::uint8_t _family, util_socket_t _fd,
        host_address _peer_addr)
        : session(CHECK_NULL(_cycle), TYPE_TCP,
            std::move(_local_addr),
            std::move(_name), _family, _fd,
            std::move(_peer_addr))
        , impl_(new tcp_session_impl)
    {
        socket().set_keep_alive(true);
    }

    void tcp_session::handle_read(const timestamp& _time)
    {
        auto read_n = d_ptr(impl_)->in_buffer_.read(fd(), -1);
        if (read_n == 0) {
            handle_close();
        } else if (read_n > 0 && d_ptr(impl_)->read_) {
            d_ptr(impl_)->read_(std::static_pointer_cast<tcp_session>(shared_from_this()), d_ptr(impl_)->in_buffer_, _time);
        } else {
            if (read_n > 0) {
                MSG_ERROR("read_callback has not been set for tcp-session[{}].", name());
            }
            handle_error();
        }
    }

    void tcp_session::handle_write()
    {
        if (event()->writing()) {
            auto write_n = d_ptr(impl_)->out_buffer_.write(fd(), -1);
            if (write_n >= 0) {
                if (d_ptr(impl_)->out_buffer_.size() == 0) {
                    event()->disable_write();
                    owner_cycle()->queue_in_cycle(std::bind([=](const wptr<tcp_session>& session) {
                        auto tcp = session.lock();
                        if (tcp) {
                            if (d_ptr(impl_)->write_) {
                                d_ptr(impl_)->write_(tcp);
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
