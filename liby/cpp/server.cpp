#include "server.h"
#include "socket.h"
#include "log.h"
#include "connection.h"

Server::Server(EVENTLOOP_PTR loop0,
               std::string server_name,
               std::string port,
               accept_handler handler0) :
    loop(loop0),
    handler(handler0),
    event(std::make_shared<Event>([this](EVENT_PTR p){ accepter(p); }))
{
    listenfd = initserver(server_name.c_str(), port.c_str());
    listenfd_ = std::make_shared<fd_holder>(listenfd);
    set_noblock(listenfd);
    event->get_event_ptr()->events = EPOLLIN | EPOLLET;
    loop->add(listenfd_, event);
}

void Server::accepter(EVENT_PTR)
{
    while(1) {
        int clfd = accept(listenfd, NULL, NULL);
        if(clfd < 0) {
            if(errno != EAGAIN) {
                log_err("accept error");
            }
            break;
        }
        set_noblock(clfd);

        auto c = std::make_shared<Connection>(shared_from_this(), clfd);
        if(handler) handler(c);
    }
}

EVENTLOOP_PTR Server::get_loop()
{
    return loop;
}
