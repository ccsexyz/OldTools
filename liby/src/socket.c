#include "socket.h"
#include "log.h"

void set_noblock(int fd) {
    int opt;
    opt = fcntl(fd, F_GETFL);
    if (opt < 0) {
        log_quit("fcntl get error!\n");
    }
    opt = opt | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, opt) < 0) {
        log_quit("fcntl set error!\n");
    }
}

void set_nonagle(int fd) {
    int nodelay = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int)) < 0) {
        log_err("disable nagle error!\n");
    }
}

int initsocket(int type, const struct sockaddr *addr, socklen_t alen,
               int qlen) {
    int fd, err;
    int reuse = 1;

    if ((fd = socket(addr->sa_family, type, 0)) < 0)
        return (-1);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
        goto errout;
    if (bind(fd, addr, alen) < 0)
        goto errout;
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)
        if (listen(fd, qlen) < 0)
            goto errout;
    return (fd);
errout:
    err = errno;
    close(fd);
    errno = err;
    return (-1);
}

int initserver(const char *server_path, const char *bind_port) {
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int sockfd, err, n;
    char *host;

    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;
    hint.ai_socktype = SOCK_STREAM;
    if ((err = getaddrinfo(server_path, bind_port, &hint, &ailist)) != 0) {
        log_err("getaddrinfo error: %s\n", gai_strerror(err));
        return -1;
    }
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sockfd = initsocket(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen,
                                 QLEN)) >= 0) {
            return sockfd;
        }
    }
    return -1;
}

static int connect_tcp_(const char *host, const char *port, int is_noblock) {
    struct addrinfo hints, *res;
    bzero((void *)(&hints), sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) == -1)
        goto errout;

    int sockfd;
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) ==
        -1)
        goto errout;

    if (is_noblock) {
        set_noblock(sockfd);
    }

    int ret;
    ret = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (ret < 0) {
        log_err("connect error: %s\n", strerror(errno));
        close(sockfd);
        goto errout;
    }

    return sockfd;

errout:
    return -1;
}

int connect_tcp(const char *host, const char *port) {
    connect_tcp_(host, port, 0);
}

int connect_tcp_noblock(const char *host, const char *port) {
    connect_tcp_(host, port, 1);
}
