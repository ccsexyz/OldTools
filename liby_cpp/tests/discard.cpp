#include "../Liby.h"

using namespace Liby;

int main() {
    EventLoop loop;
    auto discard_server = loop.creatTcpServer("localhost", "9378");
    discard_server->onAccept(
        [](std::shared_ptr<Connection> conn) { conn->suspendRead(false); });
    discard_server->onRead(
        [](std::shared_ptr<Connection> &&conn) {
            info(conn->read().retriveveAllAsString().c_str());
        });
    discard_server->start();
    loop.RunMainLoop();
    return 0;
}
