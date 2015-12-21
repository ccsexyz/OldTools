#include <iostream>
#include "liby.h"

class HelloServer : public LibyServer
{
public:
    explicit HelloServer(EVENTLOOP_PTR loop0, std::string name, std::string port) :
        LibyServer(loop0, name, port),
        str("HelloWorld!\n")
    {}
    void Accepter(std::shared_ptr<Connection> c) override
    {
        c->async_write(str.c_str(), str.length(), [this, c](BUFFER_PTR buf, int ec)
        {
            ;//do nothing
        });
    }
private:
    std::string str;
};

class EchoServer : public LibyServer
{
public:
    explicit EchoServer(EVENTLOOP_PTR loop0, std::string name, std::string port) :
        LibyServer(loop0, name, port),
        buf(std::make_shared<Buffer>())
    {}
    void Accepter(std::shared_ptr<Connection> c) override
    {
        read(c);
    }
    void read(std::shared_ptr<Connection> c)
    {
        buf->clear();
        c->async_read(buf, [this, c](BUFFER_PTR b, int ec)
        {
            if(!ec) {
                c->async_write(b, [this, c](BUFFER_PTR, int ec)
                {
                    if(!ec) {
                        read(c);
                    }
                });
            }
        });
    }
private:
    BUFFER_PTR buf;
};

using namespace std;

int main(int argc, char *argv[])
{
    auto loop = make_shared<EventLoop>();
    HelloServer h(loop, "localhost", "9377");
    EchoServer e(loop, "localhost", "9999");
    loop->main_loop();
    return 0;
}
