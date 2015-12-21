#ifndef SERVER_H
#define SERVER_H

#include "base.h"
#include "eventloop.h"

class Connection;

using accept_handler = std::function<void(std::shared_ptr<Connection>)>;

class Server : public std::enable_shared_from_this<Server>
{
public:
    explicit Server(EVENTLOOP_PTR loop0, std::string server_name, std::string port, accept_handler handler0);
    void accepter(EVENT_PTR);
    EVENTLOOP_PTR get_loop();
private:
    int listenfd;
    FD_HOLDER listenfd_;
    EVENT_PTR event;
    EVENTLOOP_PTR loop;
    accept_handler handler;
};

using SERVER_PTR = std::shared_ptr<Server>;

#endif // SERVER_H
