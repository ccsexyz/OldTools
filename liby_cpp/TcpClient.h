#ifndef POLLERTEST_TTCPCLIENT_H
#define POLLERTEST_TTCPCLIENT_H

#include "util.h"

namespace Liby {
class TcpClient final : clean_, public TimerSet {
public:
    void start();
    TcpClient &setPoller(Poller *poller) {
        poller_ = poller;
        TimerSet::setPoller(poller);
        return *this;
    }
    TcpClient &setPoller(Poller &poller) { return setPoller(&poller); }
    TcpClient &setEventLoop(EventLoop *loop) {
        loop_ = loop;
        return *this;
    }
    TcpClient &setEventLoop(EventLoop &loop) { return setEventLoop(&loop); }
    TcpClient &setSocket(Socket *s);
    TcpClient &setSocket(Socket &s) { return setSocket(&s); }
    int clientfd() const { return clientfd_; }
    TcpClient &onConnect(const ConnCallback &cb) {
        connector_ = cb;
        return *this;
    }
    TcpClient &onRead(const ConnCallback &cb) {
        readEventCallback_ = cb;
        return *this;
    }
    TcpClient &onWritAll(const ConnCallback &cb) {
        writeAllCallback_ = cb;
        return *this;
    }
    TcpClient &onErro(const ConnCallback &cb) {
        erroEventCallback_ = cb;
        return *this;
    }

public:
    void *udata_ = nullptr;

private:
    void destroy();

private:
    int clientfd_;
    Poller *poller_;
    EventLoop *loop_;
    Socket *socket_;
    FilePtr clientfp_;
    std::unique_ptr<Channel> chan_;
    ConnCallback connector_;
    ConnCallback erroEventCallback_;
    ConnCallback readEventCallback_;
    ConnCallback writeAllCallback_;
};
}

#endif // POLLERTEST_TTCPCLIENT_H
