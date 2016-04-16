#include "../Liby.h"
#include <arpa/inet.h>

using namespace Liby;
using namespace std;

class TcpProxy : clean_ {
public:
    TcpProxy(EventLoop *loop, const std::string &listenport) {
        loop_ = loop;
        server_ = loop_->creatTcpServer("localhost", listenport);
    }
    void start() {
        server_->setAcceptorCallback(
            bind(&TcpProxy::onAcceptCallback, this, std::placeholders::_1));
        server_->setReadEventCallback(bind(&TcpProxy::handleConnectionRequest,
                                           this, std::placeholders::_1));
        server_->setErroEventCallback(
            bind(&TcpProxy::onErroEventCallback, this, std::placeholders::_1));
        server_->start();
    }
    void onAcceptCallback(shared_ptr<Connection> conn) { ; }
    void handleConnectionRequest(shared_ptr<Connection> conn) {
        Buffer &rbuf = conn->read();
        if (rbuf.size() < 9)
            return;

        unsigned char ver = reinterpret_cast<unsigned char *>(rbuf.data())[0];
        unsigned char command =
            reinterpret_cast<unsigned char *>(rbuf.data())[1];
        if (ver != 4 || command != 1) {
            error("bad version or command");
            server_->closeConn(conn);
            return;
        }

        char address[50];
        if (::inet_ntop(AF_INET, reinterpret_cast<void *>(rbuf.data() + 4),
                        address, 50) == nullptr) {
            server_->closeConn(conn);
            return;
        }

        string name = address;
        string port =
            to_string(ntohs(*reinterpret_cast<uint16_t *>(rbuf.data() + 2)));

        info("name : %s, port : %s", name.data(), port.data());

        rbuf.retrieve();

        shared_ptr<TcpClient> cl =
            loop_->creatTcpClient(conn->getPoller(), name, port);
        TimerId id = cl->runAfter(
            Timestamp(8, 0), [cl] { error("fd %d timeout", cl->clientfd()); });
        cl->setConnectorCallback(
            [this, conn, id](shared_ptr<Connection> conn2) {
                conn->udata_ = new shared_ptr<Connection>(conn2);
                conn2->udata_ = new shared_ptr<Connection>(conn);
                conn->suspendRead(false);
                conn2->suspendRead(false);
                conn2->cancelTimer(id);

                char response[8];
                response[0] = 0x0;
                response[1] = 0x5A;
                conn->send(response, 8);
            });
        cl->setReadEventCallback(
            bind(&TcpProxy::onReadEventCallback, this, std::placeholders::_1));
        cl->setErroEventCallback(
            bind(&TcpProxy::onErroEventCallback, this, std::placeholders::_1));
        cl->start();
        conn->setReadCallback(
            bind(&TcpProxy::onReadEventCallback, this, std::placeholders::_1));
        conn->suspendRead();
    }
    void onReadEventCallback(shared_ptr<Connection> conn) {
        shared_ptr<Connection> *c =
            reinterpret_cast<shared_ptr<Connection> *>(conn->udata_);
        if (c) {
            (*c)->send(conn->read());
        } else {
            error("c is nil");
        }
    }
    void onErroEventCallback(shared_ptr<Connection> conn) {
        if (conn && conn->udata_) {
            auto c = reinterpret_cast<shared_ptr<Connection> *>(conn->udata_);
            (*c)->udata_ = nullptr;
            (*c)->setErro();
            delete c;
        }
    }

private:
    EventLoop *loop_ = nullptr;
    shared_ptr<TcpServer> server_ = nullptr;
};

int main(int argc, char **argv) {
    // Logger::setLevel(Logger::LogLevel::DEBUG);
    try {
        if (argc != 2) {
            throw BaseException("usage: ./tcp_proxy listenport");
        }
        EventLoop loop;
        TcpProxy proxy(&loop, argv[1]);
        proxy.start();
        loop.RunMainLoop();
    } catch (const BaseException &e) {
        cerr << e.what() << endl;
        return 0;
    }
}
