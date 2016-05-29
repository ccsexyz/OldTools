#ifndef POLLERTEST_SOCKETBASE_H
#define POLLERTEST_SOCKETBASE_H

#include "util.h"

namespace Liby {
namespace SocketOption {
enum {
    Nonagle = 0x1,
    Nonblock = 0x2,
};
};

class SocketBase : clean_ {
public:
    SocketBase() = default;

    FilePtr getFilePtr() const { return fp_; }

    int fd() const { return fd_; }

protected:
    int fd_ = -1;
    uint32_t opt_ = SocketOption::Nonblock | SocketOption::Nonagle;
    FilePtr fp_;
    std::string name_;
    std::string port_;
};
}

#endif // POLLERTEST_SOCKETBASE_H
