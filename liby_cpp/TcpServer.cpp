#include "TcpServer.h"
#include "EventLoop.h"

void TcpServer::handleAcceptEvent() {
    for (;;) {
        int clfd = ::accept(srvfd_->fd(), NULL, NULL);
        if (clfd < 0) {
            if (errno == EAGAIN) {
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                handleErroEvent();
            }
        }

        auto chanelPtr = std::make_shared<Chanel>(
            chan_->getPoller(), std::make_shared<FileHandle>(clfd), EPOLLIN);
        chanelPtr->fileHandlePtr()->setNoblock();

        if (!acceptorCallback_)
            continue;
        if (!loop_)
            continue;

        auto connectionPtr = std::make_shared<Connection>(chanelPtr);
        // conns_.push_back(std::weak_ptr<Connection>(connectionPtr));
        connectionPtr->setPoller(loop_->getSuitablePoller(clfd));
        acceptorCallback_(connectionPtr);
        connectionPtr->updateConnectionState();
    }
}
