#ifndef LIBY_EVENTLOOP_H
#define LIBY_EVENTLOOP_H

#include "Chanel.h"
#include "Poller.h"
#include "TcpClient.h"
#include "TcpServer.h"
#include <thread>
#include <vector>

class EventLoop final {
public:
    explicit EventLoop(int n = 0) : n_(n) {
        pollers_.reserve(n + 1);
        pollers_.push_back(Poller());
    }
    void RunMainLoop();
    Poller *getSuitablePoller(int fd) {
        if (n_ == 0)
            return &pollers_.front();
        else
            return &pollers_[fd % n_ + 1];
    }
    TcpServerPtr creatTcpServer(const std::string server_path,
                                const std::string server_port);
    TcpClientPtr creatTcpClient(const std::string server_path,
                                const std::string server_port);

private:
    int n_;
    std::vector<Poller> pollers_;
    std::vector<std::thread> threads_;
};

#endif // LIBY_EVENTLOOP_H
