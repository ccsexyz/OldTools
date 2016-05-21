#include "TcpClient.h"
#include "Channel.h"
#include "Connection.h"
#include "Socket.h"

void Liby::TcpClient::destroy() {
    chan_.reset();
    connector_ = nullptr;
    readEventCallback_ = nullptr;
    writeAllCallback_ = nullptr;
    erroEventCallback_ = nullptr;
    cancelAllTimer();
}

void Liby::TcpClient::start() {
    assert(loop_ && socket_ && poller_);
    chan_ = std::move(std::make_unique<Channel>(poller_, clientfp_));
    chan_->enableRead(false)
        .enableWrit()
        .onWrit([this] {
            ConnPtr conn_ = std::make_shared<Connection>(poller_, clientfp_);
            conn_->udata_ = udata_;
            conn_->onWrit(writeAllCallback_);
            conn_->onRead(readEventCallback_);
            conn_->onErro(erroEventCallback_);
            poller_->runEventHandler([this, conn_] {
                conn_->init();
                if (connector_) {
                    connector_(conn_);
                }
            });
            chan_.reset();
        })
        .onErro([this] {
            if (erroEventCallback_) {
                erroEventCallback_(std::shared_ptr<Connection>());
            }
            destroy();
        })
        .addChanel();
}

Liby::TcpClient &Liby::TcpClient::setSocket(Socket *s) {
    socket_ = s;
    clientfp_ = s->getFilePtr();
    clientfd_ = clientfp_->fd();
    return *this;
}
