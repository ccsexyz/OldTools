#ifndef POLLERTEST_TTCPSERVER_H
#define POLLERTEST_TTCPSERVER_H

#include "ServerSocket.h"
#include "util.h"

namespace Liby {
class TcpServer final : clean_ {
public:
    void start();
    TcpServer &onRead(const ConnCallback &cb) {
        readEventCallback_ = cb;
        return *this;
    }
    TcpServer &onErro(const ConnCallback &cb) {
        erroEventCallback_ = cb;
        return *this;
    }
    TcpServer &onWritAll(const ConnCallback &cb) {
        writAllCallback_ = cb;
        return *this;
    }
    TcpServer &onAccept(const ConnCallback &cb) {
        acceptorCallback_ = cb;
        return *this;
    }
    TcpServer &setPoller(Poller *poller) {
        poller_ = poller;
        return *this;
    }
    TcpServer &setPoller(Poller &poller) { return setPoller(&poller); }
    TcpServer &setEventLoop(EventLoop *loop) {
        loop_ = loop;
        return *this;
    }
    TcpServer &setEventLoop(EventLoop &loop) { return setEventLoop(&loop); }
    TcpServer &setServerSocket(ServerSocket *serverSocket);
    TcpServer &setServerSocket(ServerSocket &serverSocket) {
        return setServerSocket(&serverSocket);
    }

private:
    void initConnection(ConnPtr &connPtr);
    void handleAcceptEvent();
    void handleAcceptFd(int fd);
    void handleErroEvent();
    void handleErroEventOfConn(ConnPtr connPtr);

private:
    int listenfd_ = -1;
    FilePtr listenfp_;
    Poller *poller_;
    EventLoop *loop_;
    ServerSocket *serverSocket_;
    ConnCallback acceptorCallback_;
    ConnCallback readEventCallback_;
    ConnCallback writAllCallback_;
    ConnCallback erroEventCallback_;
    std::unique_ptr<Channel> chan_;
};
}

#endif // POLLERTEST_TTCPSERVER_H
