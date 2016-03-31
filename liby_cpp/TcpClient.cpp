#include "TcpClient.h"
#include "Chanel.h"
#include "Connection.h"
#include "EventLoop.h"
#include "File.h"
#include "Poller.h"

using namespace Liby;

TcpClient::TcpClient(const std::string &server_path,
                     const std::string &server_port) {
    clientfp_ = File::async_connect(server_path, server_port);
    clientfd_ = clientfp_->fd();
    chan_ = std::make_shared<Chanel>();
    chan_->setFilePtr(clientfp_);
}

void TcpClient::start() {
    chan_->setPoller(poller_);
    chan_->enableRead(false);
    chan_->enableWrit();
    chan_->setWritEventCallback([this] {
        conn_ = std::make_shared<Connection>(poller_, clientfp_);
        conn_->setWritCallback(writeAllCallback_);
        conn_->setReadCallback(readEventCallback_);
        conn_->setErroCallback([this](std::shared_ptr<Connection> &&conn) {
            if (erroEventCallback_) {
                erroEventCallback_(std::move(conn));
            }
            destroy();
        });
        chan_.reset();
        poller_->runEventHandler([this] {
            conn_->init();
            if (connector_) {
                connector_(conn_);
            }
        });
    });
    chan_->setErroEventCallback([this] { destroy(); });
    chan_->addChanel();
}

void TcpClient::runAsyncHandler(BasicHandler cb) {
    assert(loop_);
    loop_->getFirstPoller()->runEventHandler(cb);
}

void TcpClient::destroy() {
    clientfp_.reset();
    chan_.reset();
    conn_.reset();
    connector_ = nullptr;
    readEventCallback_ = nullptr;
    writeAllCallback_ = nullptr;
    erroEventCallback_ = nullptr;
}
