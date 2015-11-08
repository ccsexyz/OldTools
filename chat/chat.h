#include <iostream>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <vector>
#include <memory>
#include <set>
#include <map>
#include <utility>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

#define AUTHOR "ccsexyz"

class chat_server;

class chat_session
    : public std::enable_shared_from_this<chat_session>
{
public:
    chat_session(tcp::socket socket, chat_server *server, std::vector<std::shared_ptr<chat_session>> &clients)
        : socket_(std::move(socket)),
          server_(server),
          clients_(clients)
    {
        try {
            buf_ptr = std::shared_ptr<char> (new char[buffer_length], std::default_delete<char[]>());
            buf = buf_ptr.get();
        } catch(...) {
            ;
        }
    }

    void start()
    {
        do_read();
    }

    tcp::socket &socket() { return socket_; }

private: //private functions
    void do_read();

    void do_write();

    void do_release();

private:
    tcp::socket socket_;
    enum { buffer_length = 4096 };
    std::shared_ptr<char> buf_ptr;
    std::size_t length_;
    char *buf;
    int i;

    chat_server *server_;
    std::vector<std::shared_ptr<chat_session>> &clients_;
};

class chat_server
{
public:
    chat_server(boost::asio::io_service &io_service, const tcp::endpoint &endpoint)
        : acceptor_(io_service, endpoint),
          socket_(io_service)
    {
        do_accept();
    }

    void delete_this_client(chat_session *session)
    {
        for(auto it = clients_.begin(); it != clients_.end(); it++) {
            if(it->get() == session) {
                clients_.erase(it);
                return;
            }
        }
    }

    std::vector<std::shared_ptr<chat_session>> &get_clients()
    {
        return clients_;
    }
private:
    void do_accept()
    {
        acceptor_.async_accept(socket_, [this](boost::system::error_code ec)
        {
            if(!ec) {
                auto x = std::make_shared<chat_session>(std::move(socket_), this, clients_);
                clients_.emplace_back(x);
                x->start();
            }

            do_accept();
        });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    std::vector<std::shared_ptr<chat_session>> clients_;
};
