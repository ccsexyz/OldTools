#include "EventLoop.h"
#include "Poller.h"
#include "TcpClient.h"
#include "TcpServer.h"
#include <signal.h>

using namespace Liby;

EventLoop::EventLoop(int n, PollerChooser chooser) : N(n), chooser_(chooser) {
    pollers_.reserve(N + 1);
    pollers_.emplace_back(newPoller());
    for (int i = 0; i < n; i++) {
        pollers_.emplace_back(newPoller());
    }
}

EventLoop::~EventLoop() {
    for (auto &t : threads_) {
        t.join();
    }
}

std::shared_ptr<Poller> EventLoop::newPoller() {
    switch (chooser_) {
    case PollerChooser::EPOLL:
    case PollerChooser::KQUEUE:
#ifdef __linux__
        return std::make_shared<PollerEpoll>();
#elif defined(__APPLE__)
        return std::make_shared<PollerKevent>();
#endif
    case PollerChooser::POLL:
        return std::make_shared<PollerPoll>();
    case PollerChooser::SELECT:
        return std::make_shared<PollerSelect>();
    }
}

void EventLoop::RunMainLoop(std::function<bool()> cb) {
    insistFunctor_ = cb;
    Signal::signal(SIGPIPE, [] { error("receive SIGPIPE"); });
    for (int i = 0; i < N; i++) {
        threads_.emplace_back(
            std::thread([this](int index) { worker_thread(index); }, i + 1));
    }
    worker_thread(0);
    for (int i = 0; i < N; i++) {
        pollers_[i + 1]->wakeup();
    }
}

void EventLoop::worker_thread(int index) {
    assert(index >= 0 && index < pollers_.size());
    while (insistFunctor_()) {
        pollers_[index]->loop_once();
    }
}

std::shared_ptr<Poller> &EventLoop::getSuitablePoller(int fd) {
    if (N == 0) {
        return pollers_.front();
    } else {
        return pollers_[fd % N + 1];
    }
}

std::shared_ptr<TcpClient>
EventLoop::creatTcpClient(const std::string &server_path,
                          const std::string &server_port) {
    TcpClientPtr client = std::make_shared<TcpClient>(server_path, server_port);
    client->setPoller(pollers_[client->clientfd() % (N + 1)].get());
    client->setEventLoop(this);
    return client;
}

std::shared_ptr<TcpServer>
EventLoop::creatTcpServer(const std::string &server_path,
                          const std::string &server_port) {
    TcpServerPtr server = std::make_shared<TcpServer>(server_path, server_port);
    server->setPoller(pollers_.front().get());
    server->setEventLoop(this);
    return server;
}

std::shared_ptr<Poller> &EventLoop::getSuitablePoller2(int fd) {
    return pollers_[fd % (N + 1)];
}

std::shared_ptr<Poller> &EventLoop::getFirstPoller() {
    assert(pollers_.size() > 0);
    return pollers_.front();
}
