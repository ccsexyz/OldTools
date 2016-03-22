#ifndef LIBY_TCPCLIENT_H
#define LIBY_TCPCLIENT_H

#include "Chanel.h"
#include "Connection.h"

class TcpClient final {
public:
    using ConnectorCallback = std::function<void(ConnectionPtr &)>;
    TcpClient(const std::string server_path, const std::string server_port)
        : clfd_(FileHandle::async_connect(server_path, server_port)),
          chan_(std::make_shared<Chanel>()) {
        chan_->setFileHandlePtr(clfd_);
    }
    void setPoller(Poller *poller) { chan_->setPoller(poller); }
    void start() {
        chan_->setEvents(EPOLLOUT);
        chan_->setWritEventCallback(
            std::bind(&TcpClient::handleConnectEvent, this));
        chan_->setErroEventCallback(
            std::bind(&TcpClient::handleErrnoEvent, this));
        chan_->addChanelToPoller();
    }
    void setConnectorCallback(ConnectorCallback cb) { connectorCallback_ = cb; }
    void handleErrnoEvent() {
        clfd_.reset();
        chan_.reset();
    }
    void handleConnectEvent() {
        auto conn_ = std::make_shared<Connection>(chan_);
        chan_.reset();
        clfd_.reset();
        if (connectorCallback_) {
            connectorCallback_(conn_);
        }
    }
    int clfd() const { return clfd_->fd(); }

private:
    FileHandlePtr clfd_;
    ChanelPtr chan_;
    ConnectorCallback connectorCallback_;
};

using TcpClientPtr = std::shared_ptr<TcpClient>;

#endif // LIBY_TCPCLIENT_H
