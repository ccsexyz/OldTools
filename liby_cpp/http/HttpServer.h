#ifndef POLLERTEST_HTTPSERVER_H
#define POLLERTEST_HTTPSERVER_H

#include "../util.h"
#include "../TcpServer.h"
#include "Reply.h"
#include "Request.h"
#include <list>

namespace Liby {
namespace http {
class HttpServer final : clean_ {
public:
    struct HttpContext final : clean_ {
        bool keep_alive_ = true;
        Request req_;
    };

    HttpServer(const std::string &root, std::shared_ptr<TcpServer> server);
    void start();

private:
    Reply handleGetRequest(const Request &req);

    void onAcceptEventCallback(std::shared_ptr<Connection> conn);
    void onReadEventCallback(std::shared_ptr<Connection> conn);
    void onWriteAllCallback(std::shared_ptr<Connection> conn);
    void onErroEventCallback(std::shared_ptr<Connection> conn);

    long getFileSize(std::string filepath);
    int openFile(std::string filepath);

private:
    std::string root_;
    std::shared_ptr<TcpServer> http_server_;
};
}
}

#endif // POLLERTEST_HTTPSERVER_H
