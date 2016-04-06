#include "../Liby.h"

using namespace Liby;

int main() {
    EventLoop loop;
    auto daytime_server = loop.creatTcpServer("localhost", "9390");
    daytime_server->setAcceptorCallback([](std::shared_ptr<Connection> conn) {
        conn->suspendRead();
        conn->send(Buffer(Timestamp::now().toString()));
        conn->send('\n');
    });
    daytime_server->setWriteAllCallback([&daytime_server](
        std::shared_ptr<Connection> conn) { daytime_server->closeConn(conn); });
    daytime_server->start();
    loop.RunMainLoop();
    return 0;
}
