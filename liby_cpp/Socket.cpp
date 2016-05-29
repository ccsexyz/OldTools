#include "Socket.h"
#include "FileDescriptor.h"
#include <netdb.h>
#include <string.h>
#include <unistd.h>

using namespace Liby;

void Socket::connect() {
    int ret;
    struct addrinfo hints, *res;
    ::bzero((void *)(&hints), sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // TODO:
    // 异步解析
    if ((ret = ::getaddrinfo(name_.data(), port_.data(), &hints, &res)) != 0) {
        std::string errmsg = "getaddrinfo error: ";
        if (ret != EAI_SYSTEM)
            errmsg += ::gai_strerror(ret);
        else
            errmsg += ::strerror(errno);
        throw BaseException(errmsg);
    }

    DeferCaller defer([res] { ::freeaddrinfo(res); });

    int sockfd;
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) ==
        -1) {
        std::string errmsg = "socket error: ";
        errmsg += ::strerror(errno);
        throw BaseException(errmsg);
    }

    if (opt_ & SocketOption::Nonblock) {
        set_noblock(sockfd);
    }

    if (opt_ & SocketOption::Nonagle) {
        set_nonagle(sockfd);
    }

    ret = ::connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (ret < 0 && errno != EINPROGRESS) {
        std::string errmsg = "connect error: ";
        errmsg += ::strerror(errno);
        ::close(sockfd);
        throw BaseException(errmsg);
    }

    fp_ = std::make_shared<FileDescriptor>(sockfd);
}

// void Socket::sendFile(int fd, size_t size) {
//#ifdef __linux__
//    ::sendfile(fd_, fd, nullptr, size);
//#elif defined(__APPLE__)
//    ::sendfile(fd, fd_, 0, &size, nullptr, 0);
//#endif
//}
