#include "ServerSocket.h"
#include "FileDescriptor.h"
#include <netdb.h>
#include <string.h>
#include <unistd.h>

using namespace Liby;

static int initsocket(int type, const struct sockaddr *addr, socklen_t alen,
                      int qlen) noexcept {
    int fd, err;
    int reuse = 1;

    if ((fd = ::socket(addr->sa_family, type, 0)) < 0)
        return (-1);
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
        goto errout;
    }
    if (::bind(fd, addr, alen) < 0) {
        goto errout;
    }
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)
        if (::listen(fd, qlen) < 0) {
            goto errout;
        }
    return (fd);

errout:
    err = errno;
    ::close(fd);
    errno = err;
    return -1;
}

static int initserver(const char *server_path, const char *bind_port) noexcept {
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int sockfd, err;

    ::memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;
    hint.ai_socktype = SOCK_STREAM;
    if ((err = ::getaddrinfo(server_path, bind_port, &hint, &ailist)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
        return -1;
    }
    DeferCaller defer([ailist] { ::freeaddrinfo(ailist); });
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sockfd = initsocket(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen,
                                 10)) >= 0) {
            set_noblock(sockfd);
            return sockfd;
        }
    }
    return -1;
}

void Liby::ServerSocket::listen() {
    int listenfd = initserver(name_.data(), port_.data());
    if (listenfd < 0)
        throw;
    if (opt_ & SocketOption::Nonblock) {
        set_noblock(listenfd);
    }
    fp_ = std::make_shared<FileDescriptor>(listenfd);
}
