//
// Created by JohnsonJohn on 15/9/24.
//

#include "Implement.h"

void
Implement::set_noblock(int fd)
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
Implement::set_nonagle(int fd)
{
    int nodelay = 1;
    if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int)) < 0)
    {
        fprintf(stderr, "disable nagle error!\n");
    }
}

int
Implement::initsocket(int type, const struct sockaddr *addr, socklen_t alen,
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
Implement::initserver(const char *server_name, const char *bind_port)
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int             sockfd, err, n;
    char            *host;

    //if ((n = sysconf(_SC_HOST_NAME_MAX)) < 0)
    //n = HOST_NAME_MAX;  /* best guess */
    //if ((host = (char *)malloc(n)) == NULL)
    //printf("malloc error");
    /*
    if (gethostname(host, n) < 0)
        printf("gethostname error");*/

    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;
    hint.ai_socktype = SOCK_STREAM;
    if ((err = getaddrinfo(NULL, bind_port, &hint, &ailist)) != 0) {
        printf("%s: getaddrinfo error: %s", server_name,
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