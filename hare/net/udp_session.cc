#include <hare/net/udp_session.h>

#include "hare/net/socket_op.h"
#include <hare/base/io/cycle.h>
#include <hare/base/logging.h>

namespace hare {
namespace net {

    udp_session::~udp_session() = default;

    auto udp_session::append(io::buffer& _buffer) -> bool
    {
        if (state() == STATE_CONNECTED) {
            auto tmp = std::make_shared<io::buffer>();
            tmp->append(_buffer);
            owner_cycle()->queue_in_cycle(std::bind([](const wptr<udp_session>& session, io::buffer::ptr& buffer) {
                auto udp = session.lock();
                if (udp) {
                    auto out_buffer_size = udp->out_buffer_.size();
                    udp->out_buffer_.append(*buffer);
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

    auto udp_session::send(void* _bytes, size_t _length) -> bool
    {
        if (state() == STATE_CONNECTED) {
            auto tmp = std::make_shared<io::buffer>();
            decltype(_bytes) tmp_buffer = new uint8_t[_length];
            ::memcpy(tmp_buffer, _bytes, _length);
            tmp->add_block(tmp_buffer, _length);
            owner_cycle()->queue_in_cycle(std::bind([](const wptr<udp_session>& session, io::buffer::ptr& buffer) {
                auto udp = session.lock();
                if (udp) {
                    auto out_buffer_size = udp->out_buffer_.size();
                    udp->out_buffer_.append(*buffer);
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
        std::string _name, int8_t _family, util_socket_t _fd,
        host_address _peer_addr)
        : session(HARE_CHECK_NULL(_cycle), TYPE_TCP,
            std::move(_local_addr),
            std::move(_name), _family, _fd,
            std::move(_peer_addr))
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
        } else if (recv_size > 0) {
            in_buffer_.add_block(buffer, buffer_size);
            read_(std::static_pointer_cast<udp_session>(shared_from_this()), in_buffer_, _time);
        } else {
            handle_error();
        }
    }

    void udp_session::handle_write()
    {
        if (!event()->writing()) {
            LOG_TRACE() << "udp session[fd: " << fd() << ", name: " << name() << "] is down, no more writing";
            return;
        }

        void* tmp_buffer {};
        size_t buffer_size {};
        auto ret = out_buffer_.get_block(&tmp_buffer, buffer_size);

        if (ret) {
            int64_t res { 0 };
            while (buffer_size != 0) {
                res = socket_op::sendto(fd(), static_cast<uint8_t*>(tmp_buffer) + res, buffer_size,
                    peer_address().get_sockaddr(), socket_op::get_addr_len(socket().family()));
                if (res < 0) {
                    SYS_ERROR() << "sendto error to " << peer_address().to_ip_port() << ", the session will shutdown.";
                    shutdown();
                    break;
                }
                buffer_size -= res;
            }
            delete[] static_cast<uint8_t*>(tmp_buffer);
        }

        if (buffer_size >= 0) {
            if (out_buffer_.size() == 0) {
                event()->disable_write();
                owner_cycle()->queue_in_cycle(std::bind([=](const wptr<udp_session>& session) {
                    auto udp = session.lock();
                    if (udp) {
                        if (write_) {
                            write_(udp);
                        } else {
                            SYS_ERROR() << "not set write_callback to udp session[" << name() << "].";
                        }
                    }
                },
                    std::static_pointer_cast<udp_session>(shared_from_this())));
            }
            if (state() == STATE_DISCONNECTING) {
                shutdown();
            }
        } else {
            SYS_ERROR() << "an error occurred while writing the socket, detail: " << socket_op::get_socket_error_info(fd());
        }
    }

} // namespace net
} // namespace hare