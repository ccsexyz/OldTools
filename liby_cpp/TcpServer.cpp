#include "TcpServer.h"
#include "Chanel.h"
#include "Connection.h"
#include "EventLoop.h"
#include "File.h"
#include "Poller.h"

using namespace Liby;

TcpServer::TcpServer(const std::string &server_path,
                     const std::string server_port) {
    listenfp_ = File::initserver(server_path, server_port);
    if (!listenfp_) {
        throw BaseException(__FILE__, __LINE__, "initserver error");
    }
    listenfd_ = listenfp_->fd();
    chan_ = std::make_shared<Chanel>();
    chan_->setFilePtr(listenfp_);
}

void TcpServer::start() {
    chan_->enableRead();
    chan_->setPoller(poller_);
    chan_->setReadEventCallback(std::bind(&TcpServer::handleAcceptEvent, this));
    chan_->setErroEventCallback(std::bind(&TcpServer::handleErroEvent, this));
    chan_->addChanel();
}

void TcpServer::handleAcceptEvent() {
    auto ret = listenfp_->accept();
    for (auto x : ret) {
        if (x->fd() == -1) {
            handleErroEvent();
            return;
        }

        std::shared_ptr<Connection> connPtr = std::make_shared<Connection>(
            loop_->getSuitablePoller(x->fd()).get(), x);
        {
            std::unique_lock<std::mutex> G_(m_);
            conns_[x->fd()] = connPtr;
        }
        initConnection(connPtr);
        connPtr->runEventHandler([this, connPtr] {
            connPtr->init();
            if (acceptorCallback_)
                acceptorCallback_(connPtr);
        });
    }
}

void TcpServer::handleErroEvent() {
    listenfd_ = -1;
    listenfp_.reset();
    chan_.reset();
}

void TcpServer::initConnection(std::shared_ptr<Connection> &connPtr) {
    connPtr->setErroCallback(std::bind(&TcpServer::handleErroEventOfConn, this,
                                       std::placeholders::_1));
    connPtr->setReadCallback(readEventCallback_);
    connPtr->setWritCallback(writeAllCallback_);
}

void TcpServer::handleErroEventOfConn(std::shared_ptr<Connection> &&connPtr) {
    if (erroEventCallback_) {
        erroEventCallback_(std::move(connPtr));
    }
    closeConn(connPtr);
    connPtr->destroy();
}

void TcpServer::closeConn(std::shared_ptr<Connection> connPtr) {
    std::unique_lock<std::mutex> G_(m_);
    auto k_ = conns_.find(connPtr->getConnfd());
    if (k_ != conns_.end())
        conns_.erase(k_);
}

void TcpServer::runHanlderOnConns(
    std::function<void(std::shared_ptr<Connection>)> cb) {
    if (!cb) {
        error("empty ConnCallback\n");
        return;
    }
    std::unique_lock<std::mutex> G_(m_);
    for (auto &x : conns_) {
        if (x.second) {
            cb(x.second);
        }
    }
}
