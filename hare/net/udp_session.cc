#include "hare/base/fwd-inl.h"
#include <hare/base/io/cycle.h>
#include <hare/base/io/socket_op.h>
#include <hare/net/udp_session.h>

namespace hare {
namespace net {

    HARE_IMPL_DEFAULT(udp_session,
        buffer out_buffer_ {};
        buffer in_buffer_ {};
        udp_session::write_callback write_ {};
        udp_session::read_callback read_ {};
    )

    udp_session::~udp_session()
    { delete impl_; }

    void udp_session::set_read_callback(read_callback _read)
    { d_ptr(impl_)->read_ = std::move(_read); }
    void udp_session::set_write_callback(write_callback _write)
    { d_ptr(impl_)->write_ = std::move(_write); }

    auto udp_session::append(buffer& _buffer) -> bool
    {
        if (state() == STATE_CONNECTED) {
            auto tmp = std::make_shared<buffer>();
            tmp->append(_buffer);
            owner_cycle()->queue_in_cycle(std::bind([](const wptr<udp_session>& session, buffer::ptr& buffer) {
                auto udp = session.lock();
                if (udp) {
                    auto out_buffer_size = d_ptr(udp->impl_)->out_buffer_.size();
                    d_ptr(udp->impl_)->out_buffer_.append(*buffer);
                    if (out_buffer_size == 0) {
                        udp->event()->enable_write();
                        udp->handle_write();
                    }
                }
            },
                std::static_pointer_cast<udp_session>(shared_from_this()), std::move(tmp)));
            return true;
        }
        return false;
    }

    auto udp_session::send(const void* _bytes, size_t _length) -> bool
    {
        if (state() == STATE_CONNECTED) {
            auto tmp = std::make_shared<buffer>();
            auto* tmp_buffer = new uint8_t[_length];
            ::memcpy(tmp_buffer, _bytes, _length);
            tmp->add_block(tmp_buffer, _length);
            owner_cycle()->queue_in_cycle(std::bind([](const wptr<udp_session>& session, buffer::ptr& buffer) {
                auto udp = session.lock();
                if (udp) {
                    auto out_buffer_size = d_ptr(udp->impl_)->out_buffer_.size();
                    d_ptr(udp->impl_)->out_buffer_.append(*buffer);
                    if (out_buffer_size == 0) {
                        udp->event()->enable_write();
                        udp->handle_write();
                    }
                }
            },
                std::static_pointer_cast<udp_session>(shared_from_this()), std::move(tmp)));
            return true;
        }
        return false;
    }

    udp_session::udp_session(io::cycle* _cycle,
        host_address _local_addr,
        std::string _name, std::uint8_t _family, util_socket_t _fd,
        host_address _peer_addr)
        : session(CHECK_NULL(_cycle), TYPE_TCP,
            std::move(_local_addr),
            std::move(_name), _family, _fd,
            std::move(_peer_addr))
        , impl_(new udp_session_impl)
    {
    }

    void udp_session::handle_read(const timestamp& _time)
    {
        auto buffer_size = socket_op::get_bytes_readable_on_socket(fd());
        auto* buffer = new uint8_t[buffer_size];
        auto recv_size = socket_op::recvfrom(fd(), buffer, buffer_size, nullptr, 0);
        if (recv_size == 0) {
            handle_close();
            delete[] buffer;
        } else if (recv_size > 0 && d_ptr(impl_)->read_) {
            d_ptr(impl_)->in_buffer_.add_block(buffer, buffer_size);
            d_ptr(impl_)->read_(std::static_pointer_cast<udp_session>(shared_from_this()), d_ptr(impl_)->in_buffer_, _time);
        } else {
            handle_error();
        }
    }

    void udp_session::handle_write()
    {
        if (!event()->writing()) {
            MSG_TRACE("udp-session[fd={}, name={}] is down, no more writing.", fd(), name());
            return;
        }

        uint8_t* tmp_buffer {};
        size_t buffer_size {};
        auto ret = d_ptr(impl_)->out_buffer_.get_block((void**)&tmp_buffer, buffer_size);

        if (ret) {
            int64_t res { 0 };
            while (buffer_size != 0) {
                res = socket_op::sendto(fd(), tmp_buffer + res, buffer_size,
                    peer_address().get_sockaddr(), socket_op::get_addr_len(socket().family()));
                if (res < 0) {
                    MSG_ERROR("sendto error to {}, the session will shutdown.", peer_address().to_ip_port());
                    shutdown();
                    break;
                }
                buffer_size -= res;
            }
            delete[] tmp_buffer;
        }

        if (buffer_size >= 0) {
            if (d_ptr(impl_)->out_buffer_.size() == 0) {
                event()->disable_write();
                owner_cycle()->queue_in_cycle(std::bind([=](const wptr<udp_session>& session) {
                    auto udp = session.lock();
                    if (udp) {
                        if (d_ptr(impl_)->write_) {
                            d_ptr(impl_)->write_(udp);
                        } else {
                            MSG_ERROR("write_callback has not been set for udp-session[{}].", name());
                        }
                    }
                },
                    std::static_pointer_cast<udp_session>(shared_from_this())));
            }
            if (state() == STATE_DISCONNECTING) {
                shutdown();
            }
        } else {
            MSG_ERROR("an error occurred while writing the socket, detail: {}.", socket_op::get_socket_error_info(fd()));
        }
    }

    auto udp_session::in_buffer() -> buffer&
    { return d_ptr(impl_)->in_buffer_; }
    auto udp_session::read_handle() const -> const read_callback&
    { return d_ptr(impl_)->read_; }

} // namespace net
} // namespace hare