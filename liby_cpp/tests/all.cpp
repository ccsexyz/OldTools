#include "../Liby.h"

using namespace Liby;

int main(void) {
    EventLoop loop;

    auto chat_server = loop.creatTcpServer("localhost", "9379");
    chat_server->setAcceptorCallback(
        [](std::shared_ptr<Connection> conn) { conn->suspendRead(false); });
    chat_server->setReadEventCallback(
        [&chat_server](std::shared_ptr<Connection> &&conn) {
            Buffer buf(conn->read().retriveveAllAsString());
            chat_server->runHanlderOnConns(
                [buf, conn](std::shared_ptr<Connection> conn2) {
                    if (conn != conn2) {
                        conn2->send(buf.data(), buf.size());
                    }
                });
        });
    chat_server->setWriteAllCallback(
        [](std::shared_ptr<Connection> &&conn) { conn->suspendRead(false); });
    chat_server->start();

    auto daytime_server = loop.creatTcpServer("localhost", "9390");
    daytime_server->setAcceptorCallback([](std::shared_ptr<Connection> conn) {
        conn->suspendRead();
        conn->send(Buffer(Timestamp::now().toString()));
        conn->send('\n');
    });
    daytime_server->setWriteAllCallback(
        [&daytime_server](std::shared_ptr<Connection> &&conn) {
            daytime_server->closeConn(conn);
        });
    daytime_server->start();

    auto discard_server = loop.creatTcpServer("localhost", "9378");
    discard_server->setAcceptorCallback(
        [](std::shared_ptr<Connection> conn) { conn->suspendRead(false); });
    discard_server->setReadEventCallback(
        [](std::shared_ptr<Connection> &&conn) {
            info(conn->read().retriveveAllAsString().c_str());
        });
    discard_server->start();

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
