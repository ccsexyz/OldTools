#ifndef POLLERTEST_TCPCLIENT_H
#define POLLERTEST_TCPCLIENT_H

#include "util.h"

namespace Liby {
class Chanel;
class Connection;
class File;
class Poller;
class EventLoop;

class TcpClient final : clean_ {
public:
    using ConnectorCallback = std::function<void(std::shared_ptr<Connection>)>;
    using ConnCallback = std::function<void(std::shared_ptr<Connection>)>;
    TcpClient(const std::string &server_path, const std::string &server_port);
    ~TcpClient() { debug("a client deconstruct %p ", this); }
    void setPoller(Poller *poller) { poller_ = poller; }
    Poller *getPoller() const { return poller_; }
    void setEventLoop(EventLoop *loop) { loop_ = loop; }
    EventLoop *getEventLoop() const { return loop_; }
    int clientfd() const { return clientfd_; }
    void start();
    void setConnectorCallback(ConnectorCallback cb) { connector_ = cb; }
    void setReadEventCallback(ConnCallback cb) { readEventCallback_ = cb; }
    void setWriteAllCallback(ConnCallback cb) { writeAllCallback_ = cb; }
    void setErroEventCallback(ConnCallback cb) { erroEventCallback_ = cb; }
    void runAsyncHandler(BasicHandler cb);

private:
    void destroy();

public:
    void *udata_ = nullptr;

private:
    int clientfd_;
    Poller *poller_;
    EventLoop *loop_;
    std::shared_ptr<File> clientfp_;
    std::shared_ptr<Chanel> chan_;
    std::shared_ptr<Connection> conn_;
    ConnectorCallback connector_;
    ConnCallback erroEventCallback_;
    ConnCallback readEventCallback_;
    ConnCallback writeAllCallback_;
};

using TcpClientPtr = std::shared_ptr<TcpClient>;
}

#endif // POLLERTEST_TCPCLIENT_H
