#include "EventLoop.h"

static void worker_thread(Poller *poller) {
    if (poller == NULL)
        return;
    while (1) {
        // std::cout << "wtf" << std::endl;
        poller->loop_once();
    }
}

void EventLoop::RunMainLoop() {
    for (int i = 0; i < n_; i++) {
        pollers_.push_back(Poller());
        threads_.push_back(std::thread(worker_thread, &pollers_[i + 1]));
    }
    worker_thread(&pollers_.front());
}

TcpServerPtr EventLoop::creatTcpServer(const std::string server_path,
                                       const std::string server_port) {
    TcpServerPtr server = std::make_shared<TcpServer>(server_path, server_port);
    server->setPoller(&pollers_.front());
    server->setEventLoop(reinterpret_cast<EventLoop *>(this));
    return server;
}

TcpClientPtr EventLoop::creatTcpClient(const std::string server_path,
                                       const std::string server_port) {
    TcpClientPtr client = std::make_shared<TcpClient>(server_path, server_port);
    client->setPoller(&pollers_[client->clfd() % (n_ + 1)]);
    return client;
}
