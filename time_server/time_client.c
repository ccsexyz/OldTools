#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
//#include <sys/epoll.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>

int
Connect(const char *host, const char *port)
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    printf("host = %s port = %s\n", host, port);

    if(getaddrinfo(host, port, &hints, &res) == -1) {
        goto connect_error;
    }

    int sockfd;
    if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        goto connect_error;
    }

    int ret = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if(ret < 0) {
        goto connect_error;
    }

    return sockfd;

connect_error:
    return -1;
}

int main(int argc, char **argv)
{
    if(argc != 3) {
        fprintf(stderr, "usage: %s server_name server_port!\n", argv[0]);
        exit(1);
    }

    int s = Connect(argv[1], argv[2]);

    if(s < 0) {
        printf("cannot connect to the server!\n");
        exit(1);
    }

    uint32_t be32;
    int ret = read(s, &be32, sizeof be32);
    close(s);

    if(ret != sizeof be32) {
        fprintf(stderr, "read error with ret value %d\n", ret);
        exit(2);
    }

    time_t now = ntohl(be32);
    printf("TIME: %s\n", ctime(&now));

    return 0;
}
