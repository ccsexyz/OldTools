#include "../Liby.h"

using namespace Liby;
using namespace std;

class TcpRelayer : clean_ {
public:
    TcpRelayer(EventLoop *loop, const string &name, const string &port,
               const string &listenport) {
        loop_ = loop;
        name_ = name;
        port_ = port;
        server_ = loop->creatTcpServer("localhost", listenport);
    }

    void start() {
        server_->setAcceptorCallback(
            bind(&TcpRelayer::onAcceptEvent, this, std::placeholders::_1));
        server_->setReadEventCallback(bind(&TcpRelayer::onReadEventCallback,
                                           this, std::placeholders::_1));
        server_->setErroEventCallback(bind(&TcpRelayer::onErroEventCallback,
                                           this, std::placeholders::_1));
        server_->start();
    }

    void onAcceptEvent(shared_ptr<Connection> conn) {
        shared_ptr<TcpClient> cl =
            loop_->creatTcpClient(conn->getPoller(), name_, port_);
        weak_ptr<Connection> weak_conn(conn);
        conn->suspendRead();
        TimerId id = cl->getPoller()->runAfter(Timestamp(50, 0), [cl] {
            // timeout
        });
        cl->setConnectorCallback(
            [weak_conn, this, id](shared_ptr<Connection> conn) {
                if (weak_conn.expired())
                    return;
                auto conn2(weak_conn.lock());
                conn->udata_ = new shared_ptr<Connection>(conn2);
                conn2->udata_ = new shared_ptr<Connection>(conn);
                conn->suspendRead(false);
                conn2->suspendRead(false);
                conn->getPoller()->cancelTimer(id);
            });
        cl->setReadEventCallback(bind(&TcpRelayer::onReadEventCallback, this,
                                      std::placeholders::_1));
        cl->setErroEventCallback(bind(&TcpRelayer::onErroEventCallback, this,
                                      std::placeholders::_1));
        cl->start();
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
        if (conn->udata_) {
            auto c = reinterpret_cast<shared_ptr<Connection> *>(conn->udata_);
            (*c)->udata_ = nullptr;
            (*c)->setErro();
            delete c;
        }
    }

private:
    string name_;
    string port_;
    EventLoop *loop_ = nullptr;
    shared_ptr<TcpServer> server_ = nullptr;
};

int main(int argc, char **argv) {
    Logger::setLevel(Logger::LogLevel::DEBUG);
    try {
        if (argc != 4) {
            throw BaseException("usage: ./tcp_relayer name port listenport");
        }
        EventLoop loop;
        TcpRelayer relayer(&loop, argv[1], argv[2], argv[3]);
        relayer.start();
        loop.RunMainLoop();
    } catch (const BaseException &e) {
        cerr << e.what() << endl;
        return 0;
    }
}
