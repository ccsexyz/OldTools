#include "EventLoop.h"
#include "test/test.h"
#include <cstring>
#include <iostream>
#include <signal.h>

using namespace std;

int main(int argc, char **argv) {
    Signal::signal(SIGPIPE, [] { cout << "receive SIGPIPE!" << endl; });
    int num = argc == 1 ? 0 : atoi(argv[1]);
    EventLoop loop(num);
    auto server = loop.creatTcpServer("localhost", "9377");
    server->setAcceptorCallback([](ConnectionPtr &conn) {
        auto fptr = new std::function<void(char *, off_t, int)>;
        *fptr = [conn, fptr](char *buf, off_t offset, int ec) {
            if (ec == 0) {
                conn->async_writ(buf, offset,
                                 [conn, fptr](char *buf, off_t offset, int ec) {
                                     if (buf)
                                         delete[] buf;
                                     if (ec == 0)
                                         conn->async_read(*fptr);
                                     else {
                                         delete fptr;
                                     }
                                 });
            } else {
                delete fptr;
            }
        };
        conn->async_read(*fptr);
    });
    server->start();
    loop.RunMainLoop();
    return 0;
}
