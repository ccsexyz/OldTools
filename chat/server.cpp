#include "server.h"

uint32_t next_id = 1;

chat_session::chat_session(tcp::socket socket, chat_server *server)
        : socket_(std::move(socket)),
          server_(server)
{
    try {
        buf_ptr = std::shared_ptr<char> (new char[buffer_length], std::default_delete<char[]>());
        buf = buf_ptr.get();

        server_->set_session(this);
    } catch(...) {
        ;
    }
}

void
chat_session::do_read()
{
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(buf, buffer_length),
        [this, self](boost::system::error_code ec, std::size_t length)
    {
        if(ec) return;
    });
}

void
chat_session::do_read_login_message()
{
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(buf, buffer_length),
        [this, self](boost::system::error_code ec, std::size_t length)
    {
        if(ec) return;

        message_t *m = reinterpret_cast<message_t *>(buf);
        if(m->magic_word[8] != 0 ||
            std::strcmp("ccsexyz", m->magic_word) ||
            m->message_type != login_message) {
            server_->delete_this_client(this);
        }
        //uint32_t length = m->message_length;
        id_ = next_id;
        next_id++;
    });
}

void
chat_session::do_write_login_message()
{
    auto self(shared_from_this());

    message_t *m = reinterpret_cast<message_t *>(buf);
    std::memcpy(m->magic_word, "ccsexyz", 8);
    m->message_type = login_message;
    m->message_length = 0;
    m->client_id = id_;
    length_ = sizeof(message_t);

    boost::asio::async_write(socket_, boost::asio::buffer(buf, length_),
        [this, self](boost::system::error_code ec, std::size_t)
    {
        if(ec) return;

        do_read_chat_message();
    });
}

void
chat_session::do_read_chat_message()
{
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(buf, buffer_length),
        [this, self](boost::system::error_code ec, std::size_t length)
    {
        if(ec) return;

        message_t *m = reinterpret_cast<message_t *>(buf);
        if(m->magic_word[8] != 0 ||
            std::strcmp("ccsexyz", m->magic_word) ||
            m->message_type != login_message) {
            server_->delete_this_client(this);
        }

        auto weak_dest = server_->get_client(m->client_id);
        if(weak_dest.expired()) {
            //continue
            return;
        }

        auto shared_dest = weak_dest.lock();
        boost::asio::async_write(shared_dest->socket(), boost::asio::buffer(buf, length),
            [this, self](boost::system::error_code ec, std::size_t)
        {
            if(ec) return;

            length_ = 0;
            do_write_chat_message();
        });


        //std::weak_ptr<chat_session> weak_self(self);
        //dest_client->chat_queue_add(std::move(weak_self));
    });
}


int main(int argc, char **argv)
{
    try {
        if(argc < 2) {
            std::cerr << "usage: echo: chat_server <port> [<port> ... ]" << std::endl;
            return argc;
        }

        boost::asio::io_service io_service;

        std::vector<chat_server> servers;
        for(int i = 1; i < argc; i++) {
            tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
            servers.emplace_back(io_service, endpoint);
        }

        io_service.run();
    } catch(...) {
        std::cerr << "error occur!" << std::endl;
    }

    return 0;
}
