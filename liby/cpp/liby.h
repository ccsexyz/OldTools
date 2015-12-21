#ifndef LIBY_H
#define LIBY_H

#include "base.h"
#include "buffer.h"
#include "server.h"
#include "connection.h"
#include "io_task.h"
#include "eventloop.h"

class LibyServer : public std::enable_shared_from_this<LibyServer>
{
public:
    explicit LibyServer(EVENTLOOP_PTR loop0, std::string name, std::string port) :
        s(std::make_shared<Server>(loop0, name, port, std::bind(&LibyServer::Accepter, this, std::placeholders::_1)))
    {
    }

    virtual void Accepter(std::shared_ptr<Connection>) = 0;
private:
    SERVER_PTR s;
};

#endif // LIBY_H
