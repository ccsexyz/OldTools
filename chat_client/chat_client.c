#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>

int
connect_tcp(const char *host, const char *port)
{
    struct addrinfo hints, *res;
    bzero((void *)(&hints), sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if(getaddrinfo(host, port, &hints, &res) == -1)
        goto errout;

    int sockfd;
    if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
        goto errout;

    int ret;
    ret = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if(ret < 0) {
        close(sockfd);
        goto errout;
    }

    return sockfd;

errout:
    return -1;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    char *host, *port;

    if(argc == 2) {
        port = argv[1];
        host = "localhost";
    } else if(argc == 3) {
        port = argv[2];
        host = argv[1];
    } else {
        fprintf(stderr, "usage: %s <host> <port>\n", basename(argv[0]));
        exit(2);
    }

    int sockfd = connect_tcp(host, port);
    if(sockfd < 0) {
        fprintf(stderr, "Connect %s %s fail\n", host, port);
        exit(2);
    }

    int pipe_fd[2];
    ret = pipe(pipe_fd);
    if(ret != 0) {
        fprintf(stderr, "create pipe error: %s\n", strerror(errno));
        exit(ret);
    }

    int epfd = epoll_create(3);
    if(epfd < 0) {
        fprintf(stderr, "create epoll fd error: %s\n", strerror(errno));
        exit(epfd);
    }

    struct epoll_event event;
    struct epoll_event events[3];

    event.data.fd = sockfd;
    event.events = EPOLLIN | EPOLLHUP;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

    event.data.fd = STDIN_FILENO;
    event.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &event);

    char read_buffer[4096];

    while(1) {
        int i = 0;
        int nfds = epoll_wait(epfd, events, 3, -1);
        for(i = 0; i < nfds; i++) {
            if(events[i].events & EPOLLHUP) {
                printf("server close the connection!\n");
                break;
            } else if(events[i].data.fd == STDIN_FILENO) {
                ret = splice(STDIN_FILENO, NULL, pipe_fd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
                ret = splice(pipe_fd[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            } else if(events[i].data.fd == sockfd) {
                bzero((void *)read_buffer, 4096);
                ret = read(sockfd, read_buffer, 4096);
                //printf("read %d bytes!\n", ret);
                if(ret > 0) {
                    printf("%s\n", read_buffer);
                }
            }
        }
    }

    close(sockfd);
    close(epfd);
    return 0;
}
