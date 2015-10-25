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

typedef struct message_ {
    char magic_word[8];
    uint32_t message_type;
    uint32_t client_id;
    uint32_t message_length;
    char data[0];
} message_t;

extern uint32_t next_id;

class chat_server;

class chat_session
    : public std::enable_shared_from_this<chat_session>
{
public:
    chat_session(tcp::socket socket, chat_server *server);

    void start()
    {
        do_read_login_message();
    }

    //tcp::socket &&socket() { return std::move(socket_); }
    tcp::socket &socket() { return socket_; }

    void chat_queue_add(std::weak_ptr<chat_session> &&session)
    {
        send_queue.emplace_back(session);
    }

    enum message_type{ login_message, exit_message, chat_message };


private: //private functions
    void do_read()
    {
        auto self(shared_from_this());
        //socket_.async_read_some(boost::asio::buffer(buf, buffer_length),
            //[this, self](boost::system::error_code ec, std::size_t length));
    }

    void do_read_login_message();

    void do_write_login_message();

    void do_read_chat_message();

    void do_write_chat_message()
    {
        auto self(shared_from_this());
        if(send_queue.size() == 0)
        {
            ;
        }
    }

private:
    uint32_t id_;
    tcp::socket socket_;
    enum { buffer_length = 4096 };
    std::shared_ptr<char> buf_ptr;
    std::size_t length_;
    char *buf;

    std::deque<std::weak_ptr<chat_session>> send_queue;

    std::string username_;
    chat_server *server_;
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

    void set_session(chat_session *session)
    {
        if(session == nullptr) {
            std::exit(1);
        }

        client_[session] = session->shared_from_this();
    }

    void set_id(chat_session *session, uint32_t id)
    {
        auto f = client_.find(session);
        if(f != client_.end()) {
            std::weak_ptr<chat_session> x(f->second);
            client_id_[id] = x;
        }
    }

    void set_name(chat_session *session, std::string &name)
    {
        auto f = client_.find(session);
        if(f != client_.end()) {
            std::weak_ptr<chat_session> x(f->second);
            client_name_[name] = x;
        }
    }

    void delete_this_client(chat_session *session, std::string name = "", uint32_t id = 0)
    {
        auto f = client_.find(session);
        if(f != client_.end()) {
            client_.erase(f);
        }

        auto n = client_name_.find(name);
        if(n != client_name_.end()) {
            client_name_.erase(n);
        }

        auto i = client_id_.find(id);
        if(i != client_id_.end()) {
            client_id_.erase(i);
        }
    }

    std::weak_ptr<chat_session> &get_client(std::string &name)
    {
        if(name == "") {
            std::cerr << "name cannot be empty string!" << std::endl;
            std::exit(1);
        }

        auto n = client_name_.find(name);
        if(n != client_name_.end()) {
            auto &weak_client = n->second;
            return weak_client;
            /*
            if(weak_client.expired()) return;
            return weak_client.lock();*/
        }
    }

    std::weak_ptr<chat_session> &get_client(uint32_t id)
    {
        auto i = client_id_.find(id);
        if(i != client_id_.end()) {
            auto &weak_client = i->second;
            return weak_client;
            /*
            if(weak_client.expired()) return;
            return weak_client.lock();*/
        }
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(socket_, [this](boost::system::error_code ec)
        {
            if(!ec) {
                std::make_shared<chat_session>(std::move(socket_), this)->start();
            }

            do_accept();
        });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    std::map<std::string, std::weak_ptr<chat_session>> client_name_;
    std::map<uint32_t, std::weak_ptr<chat_session>> client_id_;
    std::map<chat_session *, std::shared_ptr<chat_session>> client_;
};
