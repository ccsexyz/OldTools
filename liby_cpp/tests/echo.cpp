#include "../Liby.h"
#include <iostream>

using namespace Liby;

int main() {
    EventLoop loop;
    auto echo_server = loop.creatTcpServer("localhost", "9377");
    echo_server->setAcceptorCallback(
        [](std::shared_ptr<Connection> conn) { conn->suspendRead(false); });
    echo_server->setReadEventCallback([](std::shared_ptr<Connection> &&conn) {
        conn->send(conn->read());
        conn->suspendRead();
    });
    echo_server->setWriteAllCallback(
        [](std::shared_ptr<Connection> &&conn) { conn->suspendRead(false); });
    echo_server->start();
    loop.RunMainLoop();
    return 0;
}
