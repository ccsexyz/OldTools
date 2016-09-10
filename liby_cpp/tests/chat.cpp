#include "../Liby.h"

using namespace Liby;

int main() {
    EventLoop loop;
    auto chat_server = loop.creatTcpServer("0.0.0.0", "9379");
    chat_server->onAccept(
        [](std::shared_ptr<Connection> conn) { conn->suspendRead(false); });
    chat_server->onRead(
        [&chat_server](std::shared_ptr<Connection> conn) {
            Buffer buf(conn->read().retriveveAllAsString());
//            chat_server->runHanlderOnConns(
//                [buf, conn](std::shared_ptr<Connection> conn2) {
//                    if (conn != conn2) {
//                        conn2->send(buf.data(), buf.size());
//                    }
//                });
        });
    chat_server->start();
    loop.RunMainLoop();
    return 0;
}
