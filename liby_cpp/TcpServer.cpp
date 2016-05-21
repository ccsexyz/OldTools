#include "TcpServer.h"
#include "Channel.h"
#include "Connection.h"
#include "ServerSocket.h"

void Liby::TcpServer::start() {
    assert(serverSocket_ && loop_ && poller_);
    chan_ = std::move(std::make_unique<Channel>(poller_, listenfp_));
    chan_->enableRead(true)
        .onRead([this] { handleAcceptEvent(); })
        .onErro([this] { handleErroEvent(); })
        .addChanel();
}

void Liby::TcpServer::initConnection(ConnPtr &connPtr) {
    connPtr->onRead(readEventCallback_)
        .onWrit(writAllCallback_)
        .onErro([this](ConnPtr conn) { handleErroEventOfConn(conn); });
}

void Liby::TcpServer::handleAcceptEvent() {
    assert(listenfd_ >= 0);
    for (;;) {
        int clfd = ::accept(listenfd_, NULL, NULL);
        if (clfd < 0) {
            if (errno == EAGAIN) {
                break;
            } else if (errno == EINTR) {
                continue;
            }
            handleErroEvent();
            return;
        }
        handleAcceptFd(clfd);
    }
}

void Liby::TcpServer::handleAcceptFd(int fd) {
    assert(fd >= 0);
    std::shared_ptr<Connection> connPtr =
        std::make_shared<Connection>(loop_->getSuitablePoller(fd).get(),
                                     std::make_shared<FileDescriptor>(fd));
    initConnection(connPtr);
    connPtr->runEventHandler([this, connPtr] {
        connPtr->init();
        if (acceptorCallback_)
            acceptorCallback_(connPtr);
    });
}

void Liby::TcpServer::handleErroEvent() {
    chan_.reset();
    acceptorCallback_ = nullptr;
    readEventCallback_ = nullptr;
    writAllCallback_ = nullptr;
    erroEventCallback_ = nullptr;
}

void Liby::TcpServer::handleErroEventOfConn(ConnPtr connPtr) {
    if (erroEventCallback_) {
        erroEventCallback_(connPtr);
    }
    connPtr->destroy();
}

Liby::TcpServer &Liby::TcpServer::setServerSocket(ServerSocket *serverSocket) {
    serverSocket_ = serverSocket;
    listenfp_ = serverSocket_->getFilePtr();
    listenfd_ = listenfp_->fd();
    return *this;
}
