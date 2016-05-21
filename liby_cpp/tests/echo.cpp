#include "../Liby.h"
#include <iostream>

using namespace Liby;

int main() {
    // Logger::setLevel(Logger::LogLevel::DEBUG);
    EventLoop loop(8, "EPOLL");
    auto echo_server = loop.creatTcpServer("localhost", "9377");
    echo_server->onAccept(
        [](std::shared_ptr<Connection> conn) { conn->suspendRead(false); });
    echo_server->onRead(
        [](std::shared_ptr<Connection> conn) { conn->send(conn->read()); });
    echo_server->start();
    loop.RunMainLoop();
    return 0;
}
