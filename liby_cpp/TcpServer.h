#ifndef POLLERTEST_TCPSERVER_H
#define POLLERTEST_TCPSERVER_H

#include "Connection.h"
#include "util.h"
#include <map>
#include <mutex>

namespace Liby {
class File;
class Chanel;
class Connection;
class EventLoop;
class Poller;

class TcpServer final : clean_ {
public:
    using AcceptorCallback = std::function<void(std::shared_ptr<Connection>)>;
    using ConnCallback = std::function<void(std::shared_ptr<Connection>)>;
    TcpServer(const std::string &server_path, const std::string server_port);
    void start();

    void setReadEventCallback(ConnCallback cb) { readEventCallback_ = cb; }
    void setWriteAllCallback(ConnCallback cb) { writeAllCallback_ = cb; }
    void setErroEventCallback(ConnCallback cb) { erroEventCallback_ = cb; }

    void setEventLoop(EventLoop *loop) {
        assert(loop);
        loop_ = loop;
    }
    void setAcceptorCallback(const AcceptorCallback &cb) {
        acceptorCallback_ = cb;
    }
    void setPoller(Poller *poller) { poller_ = poller; }
    int listenfd() const { return listenfd_; }
    void closeConn(std::shared_ptr<Connection> connPtr);
    void runHanlderOnConns(std::function<void(std::shared_ptr<Connection>)> cb);

private:
    void initConnection(std::shared_ptr<Connection> &connPtr);
    void handleAcceptEvent();
    void handleErroEvent();
    void handleErroEventOfConn(std::shared_ptr<Connection> connPtr);

private:
    int listenfd_;
    Poller *poller_;
    EventLoop *loop_;
    AcceptorCallback acceptorCallback_;
    ConnCallback readEventCallback_;
    ConnCallback writeAllCallback_;
    ConnCallback erroEventCallback_;
    std::shared_ptr<File> listenfp_;
    std::shared_ptr<Chanel> chan_;
    std::mutex m_;
    std::map<int, std::shared_ptr<Connection>> conns_;
};

using TcpServerPtr = std::shared_ptr<TcpServer>;
}

#endif // POLLERTEST_TCPSERVER_H
