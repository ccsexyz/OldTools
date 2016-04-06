#include "../Liby.h"
#include <iostream>

using namespace Liby;

int main() {
    // Logger::setLevel(Logger::LogLevel::DEBUG);
    EventLoop loop(0, "EPOLL");
    auto echo_server = loop.creatTcpServer("localhost", "9377");
    echo_server->setAcceptorCallback(
        [](std::shared_ptr<Connection> conn) { conn->suspendRead(false); });
    echo_server->setReadEventCallback(
        [](std::shared_ptr<Connection> conn) { conn->send(conn->read()); });
    echo_server->start();
    loop.RunMainLoop();
    return 0;
}
