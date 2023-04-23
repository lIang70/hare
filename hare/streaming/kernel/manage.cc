#include "hare/streaming/kernel/manage.h"

#include <hare/base/logging.h>
#include <hare/net/acceptor.h>
#include <hare/streaming/kernel/client.h>

namespace hare {
namespace streaming {

    struct manage::node {
        PROTOCOL_TYPE type {};
        ptr<net::acceptor> acceptor { nullptr };
        std::mutex client_mutex_ {};
        std::map<util_socket_t, ptr<client>> clents {};
    };

    manage::manage()
    {
    }

    manage::~manage()
    {
    }

    auto manage::add_node(PROTOCOL_TYPE _type, ptr<net::acceptor> _acceptor) -> bool
    {
        hare::ptr<node> one_node { new node };
        {
            std::unique_lock<std::mutex> locker(node_mutex_);
            if (nodes_.find(_acceptor->fd()) == nodes_.end()) {
                one_node->type = _type;
                one_node->acceptor = std::move(_acceptor);
                nodes_.insert(std::make_pair(one_node->acceptor->fd(), one_node));
                return true;
            }
        }

        SYS_ERROR() << "same acceptor exists in the service.";
        _acceptor->deactivate();
        return false;
    }

    auto manage::node_type(util_socket_t _accept_fd) -> PROTOCOL_TYPE
    {
        std::unique_lock<std::mutex> locker(node_mutex_);
        auto iter = nodes_.find(_accept_fd);
        return iter == nodes_.end() ? PROTOCOL_TYPE_INVALID : iter->second->type;
    }

    auto manage::add_client(util_socket_t _accept_fd, const ptr<client>& _client) -> bool
    {
        ptr<node> node {};
        auto cfd = _client->fd();
        {
            std::unique_lock<std::mutex> locker(node_mutex_);
            auto iter = nodes_.find(_accept_fd);
            if (iter == nodes_.end()) {
                SYS_ERROR() << "cannot find acceptor[" << _accept_fd << "].";
                return false;
            }
            node = iter->second;
        }

        HARE_ASSERT(node, "");

        {
            std::unique_lock<std::mutex> locker(node->client_mutex_);
            auto iter = node->clents.find(cfd);
            if (iter != node->clents.end()) {
                SYS_ERROR() << "same client[" << cfd << "] exists in the service.";
                return false;
            }
            node->clents.insert(std::make_pair(cfd, _client));
        }

        return true;
    }

    void manage::del_client(util_socket_t _accept_fd, util_socket_t _fd)
    {
        ptr<node> node {};
        {
            std::unique_lock<std::mutex> locker(node_mutex_);
            auto iter = nodes_.find(_accept_fd);
            if (iter == nodes_.end()) {
                SYS_ERROR() << "cannot find acceptor[" << _accept_fd << "].";
                return ;
            }
            node = iter->second;
        }

        HARE_ASSERT(node, "");

        {
            std::unique_lock<std::mutex> locker(node->client_mutex_);
            auto iter = node->clents.find(_fd);
            if (iter == node->clents.end()) {
                SYS_ERROR() << "cannot find client[" << _fd << "].";
                return ;
            }
            node->clents.erase(iter);
        }
    }

} // namespace streaming
} // namespace hare