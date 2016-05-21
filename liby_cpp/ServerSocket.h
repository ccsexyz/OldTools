#ifndef POLLERTEST_SERVERSOCKET_H
#define POLLERTEST_SERVERSOCKET_H

#include "SocketBase.h"

namespace Liby {

class ServerSocket final : public SocketBase {
public:
    ServerSocket() {}

    ServerSocket(const std::string &name, const std::string &port) {
        name_ = name;
        port_ = name;
    }

    ServerSocket &setName(const std::string &name) {
        name_ = name;
        return *this;
    }

    ServerSocket &setPort(const std::string &port) {
        port_ = port;
        return *this;
    }

    ServerSocket &setSocketOption(uint32_t opt) {
        opt_ |= opt;
        return *this;
    }

    ServerSocket &clearSocketOption(uint32_t opt) {
        opt_ &= (~opt);
        return *this;
    }

    void listen();
};

//    usage:
//    ServerSocket *s = new ServerSocket;
//    s->setName("0.0.0.0").setPort("80").listen();
}

#endif // POLLERTEST_SERVERSOCKET_H
