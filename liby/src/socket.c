#include "socket.h"

void
set_noblock(int fd)
{
    int opt;
    opt = fcntl(fd, F_GETFL);
    if(opt < 0) {
        perror("fcntl get error!\n");
        exit(1);
    }
    opt = opt | O_NONBLOCK;
    if(fcntl(fd, F_SETFL, opt) < 0) {
        perror("fcntl set error!\n");
        exit(1);
    }
}

void
set_nonagle(int fd)
{
    int nodelay = 1;
    if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int)) < 0)
    {
        fprintf(stderr, "disable nagle error!\n");
    }
}

int
initsocket(int type, const struct sockaddr *addr, socklen_t alen,
                      int qlen)
{
    int fd, err;
    int reuse = 1;

    if ((fd = socket(addr->sa_family, type, 0)) < 0)
        return(-1);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                   sizeof(int)) < 0) {
        err = errno;
        close(fd);
        errno = err;
        return(-1);
    }
    if (bind(fd, addr, alen) < 0) {
        err = errno;
        close(fd);
        errno = err;
        return(-1);
    }
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)
    if (listen(fd, qlen) < 0) {
        err = errno;
        close(fd);
        errno = err;
        return(-1);
    }
    return(fd);
}

int
initserver(const char *server_path, const char *bind_port)
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int             sockfd, err, n;
    char            *host;

    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;
    hint.ai_socktype = SOCK_STREAM;
    if ((err = getaddrinfo(server_path, bind_port, &hint, &ailist)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n",
               gai_strerror(err));
        return -1;
    }
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sockfd = initsocket(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen, QLEN)) >= 0) {
            return sockfd;
        }
    }
    return -1;
}
