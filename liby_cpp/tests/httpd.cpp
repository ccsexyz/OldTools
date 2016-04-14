#include "../Liby.h"
#include "../http/HttpServer.h"

using namespace Liby;
using namespace Liby::http;

int main(void) {
    //    Logger::setLevel(Logger::LogLevel::DEBUG);
    EventLoop loop(8, "EPOLL");
    HttpServer httpd("/var/www/html", loop.creatTcpServer("localhost", "9377"));
    httpd.start();
    loop.RunMainLoop();
}
