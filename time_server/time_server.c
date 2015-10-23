// Linux Only

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>

static const int QLEN = 10;

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
initserver(const char *server_name, const char *bind_port)
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int             sockfd, err, n;
    char            *host;

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

int main(int argc, char **argv)
{
    int listen_fd = initserver("yxx", "9377");
    set_noblock(listen_fd);
    int epoll_fd = epoll_create(10240);
    if(epoll_fd < 0) {
        fprintf(stderr, "epoll_create fail!\n");
        exit(1);
    }
    struct epoll_event event;
    event.data.fd = listen_fd;
    event.events = EPOLLIN | EPOLLERR | EPOLLET;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) < 0) {
        fprintf(stderr, "epoll_add fail!\n");
        exit(1);
    }

    struct epoll_event events[10240];
    for(int nfds = 0; ; ) {
        nfds = epoll_wait(epoll_fd, events, 10240, -1);
        if(nfds <= 0) {
            fprintf(stderr, "epoll_wait error!\n");
            exit(2);
        }

        for(int i = 0; i < nfds; i++) {
            struct epoll_event *ev = &(events[i]);
            if(ev->data.fd == listen_fd) {
                while(1) {
                    int clfd = accept(listen_fd, NULL, NULL);
                    if(clfd < 0) {
                        if(errno == EAGAIN) {
                            break;
                        } else {
                            fprintf(stderr, "accept error : ", strerror(errno));
                            break;
                        }
                    }

                    set_noblock(clfd);

                    event.data.fd = clfd;
                    event.events = EPOLLOUT | EPOLLERR | EPOLLET;
                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clfd, &event) < 0) {
                        fprintf(stderr, "epoll_add fail!\n");
                        exit(1);
                    }
                }
            } else if(ev->events & EPOLLERR | ev->events &EPOLLHUP) {
                close(ev->data.fd);
                continue;
            } else if(ev->events & EPOLLOUT) {
                time_t now = time(NULL);
                uint32_t be32 = htonl(now);
                int ret = write(ev->data.fd, &be32, sizeof be32);
                close(ev->data.fd);
            }
        }
    }
}
