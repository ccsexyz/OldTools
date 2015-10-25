#include "client.h"

int main(int argc, char *argv[])
{
    try {
        if(argc != 3) {
            std::cerr << "usage: chat_client <host> <port>" << std::endl;
            return 1;
        }

        asio::io_service io_service;
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(argv[1], argv[2]);
        tcp::resolver::iterator iterator = resolver.resolve(query);

        chat_client client(io_service, iterator);
        asio::thread thread(boost::bind(&asio::io_service::run, &io_service));

    } catch(std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
