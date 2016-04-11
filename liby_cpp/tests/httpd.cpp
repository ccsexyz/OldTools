#include "../Liby.h"
#include "../http/HttpServer.h"

using namespace Liby;
using namespace Liby::http;

int main(void) {
    //    Logger::setLevel(Logger::LogLevel::DEBUG);
    EventLoop loop(0, "EPOLL");
    HttpServer httpd("/home/ccsexyz", loop.creatTcpServer("localhost", "9377"));
    httpd.start();
    loop.RunMainLoop();
}
