#ifndef POLLERTEST_EVENTLOOP_H
#define POLLERTEST_EVENTLOOP_H

#include "util.h"
#include <thread>
#include <vector>

namespace Liby {
class Poller;
class PollerEpoll;
class PollerPoll;
class PollerSelect;
class TcpServer;
class TcpClient;

class EventLoop final : clean_ {
public:
    enum class PollerChooser { EPOLL, POLL, SELECT, KQUEUE };
    explicit EventLoop(int n = 0, PollerChooser chooser = PollerChooser::EPOLL);
    ~EventLoop();
    void RunMainLoop(std::function<bool()> cb = [] { return true; });
    std::shared_ptr<Poller> &getSuitablePoller(int fd);
    std::shared_ptr<Poller> &getSuitablePoller2(int fd);
    std::shared_ptr<Poller> &getFirstPoller();
    std::shared_ptr<TcpServer> creatTcpServer(const std::string &server_path,
                                              const std::string &server_port);
    std::shared_ptr<TcpClient> creatTcpClient(const std::string &server_path,
                                              const std::string &server_port);

private:
    std::shared_ptr<Poller> newPoller();
    void worker_thread(int index);

private:
    int N;
    PollerChooser chooser_;
    std::function<bool()> insistFunctor_;
    std::vector<std::shared_ptr<Poller>> pollers_;
    std::vector<std::thread> threads_;
};
}

#endif // POLLERTEST_EVENTLOOP_H
