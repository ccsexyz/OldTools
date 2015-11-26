#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>
#include <netdb.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <errno.h>

#define BUFLEN		128
#define TIMEOUT		20
#define MAXADDRLEN	256

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

#define UNIXEPOCH 2208988800

int
initserver(int type, const struct sockaddr *addr, socklen_t alen,
           int qlen)
{
    int fd, err;
    int reuse = 1;

    if ((fd = socket(addr->sa_family, type, 0)) < 0)
        return(-1);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                   sizeof(int)) < 0)
        goto errout;
    if (bind(fd, addr, alen) < 0)
        goto errout;
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)
        if (listen(fd, qlen) < 0)
            goto errout;
    return(fd);

errout:
    err = errno;
    close(fd);
    errno = err;
    return(-1);
}

void
serve(int sockfd)
{
    int				n;
    socklen_t		alen;
    FILE			*fp;
    char			buf[BUFLEN];
    char			abuf[MAXADDRLEN];
    struct sockaddr	*addr = (struct sockaddr *)abuf;

    for (;;) {
        alen = MAXADDRLEN;
        printf("for!\n");
        if ((n = recvfrom(sockfd, buf, 1, 0, addr, &alen)) < 0) {
            syslog(LOG_ERR, "timed: recvfrom error: %s",
                   strerror(errno));
            exit(1);
        }

        time_t now = time(NULL);
        *(uint32_t *)(buf) = htonl((uint32_t)(now) + UNIXEPOCH);
        sendto(sockfd, buf, sizeof(uint32_t), 0, addr, alen);
    }
}

void
err_quit(const char *str)
{
    fprintf(stderr, "%s\n", str);
    exit(1);
}

void
err_sys(const char *str)
{
    err_quit(str);
}

int
main(int argc, char *argv[])
{
    struct addrinfo	*ailist, *aip;
    struct addrinfo	hint;
    int				sockfd, err, n;
    char			*host;

    if (argc != 1)
        err_quit("usage: timed");
    if ((n = sysconf(_SC_HOST_NAME_MAX)) < 0)
        n = HOST_NAME_MAX;	/* best guess */
    if ((host = malloc(n)) == NULL)
        err_sys("malloc error");
    if (gethostname(host, n) < 0)
        err_sys("gethostname error");

    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_CANONNAME;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;
    if ((err = getaddrinfo("localhost", "time", &hint, &ailist)) != 0) {
        err_sys("timed: getaddrinfo error");
    }
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sockfd = initserver(SOCK_DGRAM, aip->ai_addr,
                                 aip->ai_addrlen, 0)) >= 0) {
            serve(sockfd);
            exit(0);
        }
    }
    printf("exit!\n");
    exit(1);
}
