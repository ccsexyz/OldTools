#include "../Liby.h"

using namespace Liby;

int main() {
    EventLoop loop;
    auto chat_server = loop.creatTcpServer("localhost", "9379");
    chat_server->setAcceptorCallback(
        [](std::shared_ptr<Connection> conn) { conn->suspendRead(false); });
    chat_server->setReadEventCallback(
        [&chat_server](std::shared_ptr<Connection> conn) {
            Buffer buf(conn->read().retriveveAllAsString());
            chat_server->runHanlderOnConns(
                [buf, conn](std::shared_ptr<Connection> conn2) {
                    if (conn != conn2) {
                        conn2->send(buf.data(), buf.size());
                    }
                });
        });
    chat_server->start();
    loop.RunMainLoop();
    return 0;
}
