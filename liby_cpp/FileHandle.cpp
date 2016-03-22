#include "FileHandle.h"
#include <cstring>
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#define QLEN (10)

static void set_noblock(int fd, bool flag = true) {
    int opt;
    opt = ::fcntl(fd, F_GETFL);
    if (opt < 0) {
        perror("fcntl get error!\n");
        exit(1);
    }
    opt = flag ? (opt | O_NONBLOCK) : (opt & ~O_NONBLOCK);
    if (::fcntl(fd, F_SETFL, opt) < 0) {
        perror("fcntl set error!\n");
        exit(1);
    }
}

void FileHandle::setNoblock(bool flag) { set_noblock(fd_, flag); }

void FileHandle::setNonagle(bool flag) {
    int nodelay = flag ? 1 : 0;
    if (::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int)) <
        0) {
        fprintf(stderr, "disable nagle error!\n");
    }
}

static int initsocket(int type, const struct sockaddr *addr, socklen_t alen,
                      int qlen) {
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
    close(fd);
    errno = err;
    return -1;
}

static int initserver(const char *server_path, const char *bind_port) {
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int sockfd, err, n;
    char *host;

    ::memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;
    hint.ai_socktype = SOCK_STREAM;
    if ((err = ::getaddrinfo(server_path, bind_port, &hint, &ailist)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
        return -1;
    }
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sockfd = initsocket(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen,
                                 QLEN)) >= 0) {
            set_noblock(sockfd);
            return sockfd;
        }
    }
    return -1;
}

FileHandlePtr FileHandle::initserver(const std::string &server_path,
                                     const std::string &server_port) {
    return std::make_shared<FileHandle>(
        ::initserver(server_path.data(), server_port.data()));
}

static int connect_tcp(const char *host, const char *port, int is_noblock) {
    struct addrinfo hints, *res;
    ::bzero((void *)(&hints), sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) == -1)
        goto errout;

    int sockfd;
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) ==
        -1)
        goto errout;

    if (is_noblock) {
        set_noblock(sockfd, true);
    }

    int ret;
    ret = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (ret < 0) {
        printf("connect error: %s\n", ::strerror(errno));
        close(sockfd);
        goto errout;
    }

    return sockfd;

errout:
    return -1;
}

FileHandlePtr FileHandle::async_connect(const std::string &host,
                                        const std::string &port,
                                        bool isNoblock) {
    return std::make_shared<FileHandle>(
        connect_tcp(host.data(), port.data(), isNoblock));
}

ssize_t FileHandle::read(void *buf, size_t nbyte) {
    int ret = ::read(fd_, buf, nbyte);
    savedErrno_ = errno;
    return ret;
}

ssize_t FileHandle::write(void *buf, size_t nbyte) {
    int ret = ::write(fd_, buf, nbyte);
    savedErrno_ = errno;
    return ret;
}

void FileHandle::tryCloseFd() const {
    if (fd_ >= 0) {
        ::close(fd_);
    }
}
