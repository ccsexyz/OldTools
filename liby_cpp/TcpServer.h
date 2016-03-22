#ifndef LIBY_TCPSERVER_H
#define LIBY_TCPSERVER_H

#include "Chanel.h"
#include "Connection.h"
#include "FileHandle.h"
#include <iostream>
#include <sys/socket.h>

class EventLoop;

class TcpServer final {
public:
    using AcceptorCallback = std::function<void(ConnectionPtr &)>;
    TcpServer(const std::string server_path, const std::string server_port)
        : srvfd_(FileHandle::initserver(server_path, server_port)),
          chan_(std::make_shared<Chanel>()) {
        chan_->setFileHandlePtr(srvfd_);
    }
    void setPoller(Poller *poller) { chan_->setPoller(poller); }
    void start() {
        chan_->setEvents(EPOLLIN);
        chan_->setReadEventCallback(
            std::bind(&TcpServer::handleAcceptEvent, this));
        chan_->setErroEventCallback(
            std::bind(&TcpServer::handleErroEvent, this));
        chan_->addChanelToPoller();
    }
    void handleAcceptEvent();
    void handleErroEvent() {
        srvfd_.reset();
        chan_.reset();
    }
    void setAcceptorCallback(AcceptorCallback cb) { acceptorCallback_ = cb; }
    int srvfd() const { return srvfd_->fd(); }
    void setEventLoop(EventLoop *loop) { loop_ = loop; }

private:
    FileHandlePtr srvfd_;
    ChanelPtr chan_;
    EventLoop *loop_ = nullptr;
    AcceptorCallback acceptorCallback_;
    BlockingQueue<std::weak_ptr<Connection>> conns_;
};

using TcpServerPtr = std::shared_ptr<TcpServer>;

#endif // LIBY_TCPSERVER_H
