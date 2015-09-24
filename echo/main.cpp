#include <iostream>
#include "Implement.h"
#include "Epoll.h"

#define DEFAULT_SERVER_NAME "echo"
#define DEFAULT_BIND_PORT   "9377"

int main() {
    int listenfd = Implement::initserver(DEFAULT_SERVER_NAME, DEFAULT_BIND_PORT);
    if(listenfd <= 0) {
        std::cerr << "init error!" << std::endl << "check the port please!" << std::endl;
        std::exit(1);
    }

    auto handle_newcon = [&listenfd]()
        {
            while(1) {
                int clfd = accept(listenfd, NULL, NULL);
                if(clfd < 0) {
                    if(errno == EAGAIN) {
                        break;
                    } else {
                        std::cerr << "accept error : " << strerror(errno) << std::endl;
                        break;
                    }
                }
                Implement::set_noblock(clfd);
            }
        };

    Epoll *ep = new Epoll(0);


    return 0;
}