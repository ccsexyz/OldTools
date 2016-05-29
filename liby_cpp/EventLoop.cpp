#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"
#include "ServerSocket.h"
#include "Socket.h"
#include "TcpClient.h"
#include "TcpServer.h"
#include <signal.h>

using namespace Liby;

std::map<std::string, EventLoop::PollerChooser> EventLoop::ChooserStrings = {
    {"EPOLL", PollerChooser::EPOLL},
    {"POLL", PollerChooser::POLL},
    {"SELECT", PollerChooser::SELECT},
    {"KQUEUE", PollerChooser::KQUEUE}};

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
        break;
    case PollerChooser::POLL:
        return std::make_shared<PollerPoll>();
    case PollerChooser::SELECT:
        return std::make_shared<PollerSelect>();
    }
#ifdef __linux__
    return std::make_shared<PollerEpoll>();
#elif defined(__APPLE__)
    return std::make_shared<PollerKevent>();
#endif
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
    assert(index >= 0 &&
           static_cast<decltype(pollers_.size())>(index) < pollers_.size());

    pollers_[index]->init();
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
    auto s = std::make_shared<Socket>();
    TcpClientPtr client;
    try {
        s->setName(server_path).setPort(server_port).connect();
        client = std::make_shared<TcpClient>();
        client->setPoller(pollers_[client->clientfd() % (N + 1)].get())
            .setEventLoop(this)
            .setSocket(s.get());
        ExitCaller::call([s] {});
    } catch (const std::exception &e) { error("%s", e.what()); }
    return client;
}

std::shared_ptr<TcpClient>
EventLoop::creatTcpClient(Poller *poller, const std::string &server_path,
                          const std::string &server_port) {
    auto s = std::make_shared<Socket>();
    TcpClientPtr client;
    try {
        s->setName(server_path).setPort(server_port).connect();
        client = std::make_shared<TcpClient>();
        client->setPoller(poller).setEventLoop(this).setSocket(s.get());
        ExitCaller::call([s] {});
    } catch (const std::exception &e) { error("%s", e.what()); }
    return client;
}

std::shared_ptr<TcpServer>
EventLoop::creatTcpServer(const std::string &server_path,
                          const std::string &server_port) {
    auto ss = std::make_shared<ServerSocket>();
    TcpServerPtr server;
    try {
        server = std::make_shared<TcpServer>();
        ss->setName(server_path).setPort(server_port).listen();
        server->setEventLoop(this)
            .setPoller(pollers_.front().get())
            .setServerSocket(ss.get());
        ExitCaller::call([ss] {});
    } catch (const std::exception &e) { error("%s", e.what()); }
    return server;
}

std::shared_ptr<Poller> &EventLoop::getSuitablePoller2(int fd) {
    return pollers_[fd % (N + 1)];
}

std::shared_ptr<Poller> &EventLoop::getFirstPoller() {
    assert(pollers_.size() > 0);
    return pollers_.front();
}
