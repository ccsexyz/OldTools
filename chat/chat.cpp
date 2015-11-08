#include "chat.h"

void
chat_session::do_read()
{
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(buf, buffer_length),
        [this, self](boost::system::error_code ec, std::size_t length)
    {
        if(ec) {
            do_release();
            return;
        }

        i = 0;
        for(auto it = clients_.begin(); it != clients_.end(); it++, i++) {
            auto it_ptr = it->get();
            if(it_ptr == this) continue;

            boost::asio::async_write(it_ptr->socket(), boost::asio::buffer(buf, length),
                [this, self, it_ptr](boost::system::error_code ec, std::size_t)
            {
                if(ec) {
                    return;
                }

                do_write();
            });
        }
    });
}

void
chat_session::do_write()
{
    i--;
    if(i == 1) do_read();
}

void
chat_session::do_release()
{
    server_->delete_this_client(this);
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
