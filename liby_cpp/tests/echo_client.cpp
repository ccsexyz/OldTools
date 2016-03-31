#include "../Liby.h"
#include <iostream>
#include <map>
#include <vector>
using namespace Liby;
using namespace std;

void print_usage() {
    cerr << "usage: ./echo_client server_name server_port concurrency "
            "[active_clients] msg_num, msg_len"
         << endl;
    ::exit(1);
}

int main(int argc, char **argv) {
    Logger::setLevel(Logger::LogLevel::INFO);
    int msg_num, msg_len, concurrency, active_clients;
    if (argc == 7) {
        msg_num = ::atoi(argv[5]);
        msg_len = ::atoi(argv[6]);
        concurrency = ::atoi(argv[3]);
        active_clients = ::atoi(argv[4]);
    } else if (argc == 6) {
        msg_num = ::atoi(argv[4]);
        msg_len = ::atoi(argv[5]);
        active_clients = concurrency = ::atoi(argv[3]);
    } else {
        print_usage();
    }
    unsigned long bytesPerClients = msg_len * msg_num;
    unsigned long totalBytes = bytesPerClients * active_clients;

    vector<char> buf(msg_len, 'A');
    vector<unsigned long> bytes(concurrency, 0);

    EventLoop loop(8, EventLoop::PollerChooser::EPOLL);
    map<TcpClient *, shared_ptr<TcpClient>> clients;

    auto start = Timestamp::now();
    for (int i = 0; i < concurrency; i++) {
        auto echo_client = loop.creatTcpClient(argv[1], argv[2]);
        if (i < active_clients) {
            unsigned long *pBytes = &bytes[i];
            auto echo_client_ = echo_client.get();
            echo_client->setConnectorCallback(
                [&buf](std::shared_ptr<Connection> conn) {
                    conn->suspendWrit(false);
                    conn->send(&buf[0], buf.size());
                });
            echo_client->setReadEventCallback(
                [&buf, pBytes, &active_clients, &loop, bytesPerClients,
                 echo_client_, &clients](std::shared_ptr<Connection> &&conn) {
                    *pBytes += conn->read().size();
                    conn->send(conn->read());

                    if (*pBytes >= bytesPerClients) {
                        conn->destroy();
                        loop.getFirstPoller()->runEventHandler(
                            [&active_clients, echo_client_, &clients] {
                                clients.erase(echo_client_);
                                active_clients--;
                            });
                    }
                });
            echo_client->setWriteAllCallback(
                [&buf](std::shared_ptr<Connection> &&conn) {
                    conn->suspendRead(false);
                    conn->suspendWrit();
                });
            echo_client->setErroEventCallback(
                [&loop, &active_clients](std::shared_ptr<Connection> &&) {
                    loop.getFirstPoller()->runEventHandler(
                        [&active_clients] { active_clients--; });
                });
        }
        echo_client->start();
        clients[echo_client.get()] = echo_client;
    }
    loop.RunMainLoop([&active_clients] { return active_clients > 0; });
    auto end = Timestamp::now();
    info("总时间 %g 秒", (end - start).toSecF());
    info("totalBytes = %ld", totalBytes);
    info("速度 %lf MiB/s", totalBytes / (end - start).toSecF() / 1024 / 1024);
    return 0;
}
