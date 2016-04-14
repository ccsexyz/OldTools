#include "HttpServer.h"
#include "../Connection.h"
#include "../File.h"
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace Liby;
using namespace Liby::http;

HttpServer::HttpServer(const std::string &root,
                       std::shared_ptr<TcpServer> server)
    : root_(root), http_server_(server) {
    ;
}

void HttpServer::start() {
    http_server_->setReadEventCallback(std::bind(
        &HttpServer::onReadEventCallback, this, std::placeholders::_1));
    http_server_->setWriteAllCallback(std::bind(&HttpServer::onWriteAllCallback,
                                                this, std::placeholders::_1));
    http_server_->setAcceptorCallback(std::bind(
        &HttpServer::onAcceptEventCallback, this, std::placeholders::_1));
    http_server_->setErroEventCallback(std::bind(
        &HttpServer::onErroEventCallback, this, std::placeholders::_1));
    http_server_->start();
}

void HttpServer::onAcceptEventCallback(std::shared_ptr<Connection> conn) {
    assert(conn);
    HttpContext *cxt = new (std::nothrow) HttpContext;
    if (cxt == nullptr) {
        error("alloc error");
        http_server_->closeConn(conn);
        conn->destroy();
    } else {
        conn->udata_ = reinterpret_cast<void *>(cxt);
    }
}

void HttpServer::onReadEventCallback(std::shared_ptr<Connection> conn) {
    assert(conn && conn->udata_);
    HttpContext *context = reinterpret_cast<HttpContext *>(conn->udata_);
    while (1) {
        Buffer &readBuf = conn->read();
        if (readBuf.size() == 0)
            return;
        int savedBytes = context->req_.bytes_;
        context->req_.parse(readBuf.data(), readBuf.data() + readBuf.size());
        readBuf.retrieve(context->req_.bytes_ - savedBytes);
        // context->req_.print();
        if (context->req_.isError()) {
            info("Request is Error!");
            Reply rep;
            rep.status_ = Reply::BAD_REQUEST;
            rep.headers_["Date"] = Timestamp::now().toString();
            conn->send(rep.toString());
            context->keep_alive_ = false;
            break;
        } else if (context->req_.isGood()) {
            // info("Request is Good!");
            Reply rep;
            // rep.headers_["Date"] = Timestamp::now().toString();
            rep.filepath_ = root_ + context->req_.uri_;
            long filesize = getFileSize(rep.filepath_);
            int fd = openFile(rep.filepath_);
            if (filesize == -1 || fd < 0) {
                rep.status_ = Reply::NOT_FOUND;
            } else {
                rep.filefp_ = std::make_shared<File>(fd);
                rep.filesize_ = filesize;
                rep.filefd_ = fd;
                rep.status_ = Reply::OK;
                rep.headers_["Content-Length"] = std::to_string(filesize);
                rep.headers_["Content-Type"] = rep.getMimeType();
            }
            conn->send(rep.toString());
            if (rep.filefp_)
                conn->sendFile(rep.filefp_, 0, rep.filesize_);
            context->req_.clear();
            if (filesize < 0 || fd < 0 ||
                context->req_.headers_["Connection"] != "keep-alive") {
                context->keep_alive_ = false;
                break;
            }
        } else {
            info("Request isn't complete!");
            break;
        }
    }
}

void HttpServer::onWriteAllCallback(std::shared_ptr<Connection> conn) {
    assert(conn && conn->udata_);
    HttpContext *context = reinterpret_cast<HttpContext *>(conn->udata_);
    if (context->keep_alive_ == false) {
        delete context;
        http_server_->closeConn(conn);
        conn->destroy();
    }
}

void HttpServer::onErroEventCallback(std::shared_ptr<Connection> conn) {
    if (conn && conn->udata_) {
        HttpContext *context = reinterpret_cast<HttpContext *>(conn->udata_);
        delete context;
        conn->udata_ = nullptr;
    }
}

long HttpServer::getFileSize(std::string filepath) {
    if (filepath.empty())
        return -1;

    struct stat st;
    int ret = ::stat(filepath.data(), &st);
    if (ret == -1) {
        error("stat %s error: %s", filepath.data(), ::strerror(errno));
        return -1;
    }
    return st.st_size;
}

int HttpServer::openFile(std::string filepath) {
    if (filepath.empty())
        return -1;

    int fd = ::open(filepath.data(), O_RDONLY);
    if (fd < 0) {
        error("open %s error: %s", filepath.data(), ::strerror(errno));
    }
    return fd;
}

// Reply handleGetRequest(const Request &req) {
//    Reply ret;
//
//    if(req.isGood()) {
//        ret.filepath_ = root_ + req.uri_;
//        int fd = openFile(ret.filepath_);
//        long filesize = getFileSize(ret.filepath_);
//        if(fd < 0) {
//            ret.status_ = 404;
//            ret.genContent();
//        } else {
//            ret.status_ = 200;
//            ret.filefd_ = fd;
//            ret.filesize_ = filesize;
//            ret.headers_["Content-Type"] = ret.getMimeType();
//        }
//    } else if(req.isError()) {
//        ret.status_ = 404;
//        ret.genContent();
//    } else {
//        ;
//    }
//    return ret;
//}
