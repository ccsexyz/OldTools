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

    // n的意义为工作线程数,总线程数为n + 1,每个线程一个事件驱动循环,默认为单线程
    // 其中第一个线程有特殊意义,负责服务器端接收连接和运行不需要立即执行的临界区代码

    explicit EventLoop(int n = 0, PollerChooser chooser = PollerChooser::EPOLL);
    ~EventLoop();
    void RunMainLoop(std::function<bool()> cb = [] { return true; });
    std::shared_ptr<Poller> &getSuitablePoller(int fd);
    std::shared_ptr<Poller> &getSuitablePoller2(int fd);
    std::shared_ptr<Poller> &getFirstPoller();

    //每个TcpServer都跑在第一个线程中,但是TcpServer中的AcceptorCallback在其创建的Connection所属线程中被回调

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
