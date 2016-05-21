#ifndef POLLERTEST_SOCKET_H
#define POLLERTEST_SOCKET_H

#include "SocketBase.h"
#include <deque>

namespace Liby {

//    usage:
//    auto s = (new Socket)->setName("0.0.0.0").setPort("80").connect();

class Socket final : public SocketBase {
public:
    Socket() {}

    Socket(const std::string &name, const std::string &port) {
        name_ = name;
        port_ = port;
    }

    Socket &setName(const std::string &name) {
        name_ = name;
        return *this;
    }

    Socket &setPort(const std::string &port) {
        port_ = port;
        return *this;
    }

    Socket &setSocketOption(uint32_t opt) {
        opt_ |= opt;
        return *this;
    }

    Socket &clearSocketOption(uint32_t opt) {
        opt_ &= (~opt);
        return *this;
    }

    void connect();
};
}

#endif // POLLERTEST_SOCKET_H
