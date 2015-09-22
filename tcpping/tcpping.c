/*************************************************************************
	> File Name: tcpping.c
	> Author:
	> Mail:
	> Created Time: 2015年09月19日 星期六 20时47分53秒
 ************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if defined(__linux__) || defined (__linux)
#include <sys/epoll.h>
#elif defined(__APPLE__) && defined (__MACH__)
#include <sys/event.h>
#endif
#include <sys/time.h>
#include <errno.h>


struct addrinfo hints, *res;
struct timeval oldtime, newtime;

char pr_str[200];

void
usage(const char *name0)
{
	printf("usage: %s path port!\n", name0);
	exit(1);
}

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
print(void)
{
	char buf[300];
	gettimeofday(&newtime, NULL);
	time_t tv_sec;
	suseconds_t tv_usec;
	if(newtime.tv_usec > oldtime.tv_usec) {
		tv_usec = newtime.tv_usec - oldtime.tv_usec;
		tv_sec = newtime.tv_sec - oldtime.tv_sec;
	} else {
		tv_usec = newtime.tv_usec + 1000000L - oldtime.tv_usec;
		tv_sec = newtime.tv_sec - oldtime.tv_sec - 1;
	}
	if(tv_sec == 0) {
		sprintf(buf, "%s%d ms", pr_str, tv_usec / 1000);
	} else {
		sprintf(buf, "%s%d ms", pr_str, tv_usec / 1000 + tv_sec);
	}
	puts(buf);
	sleep(1);
}

int
nonb_connect(int fd)
{
	gettimeofday(&oldtime, NULL);
	int n = connect(fd, res->ai_addr, res->ai_addrlen);
	if(n < 0) {
		if(errno != EINPROGRESS) {
			fprintf(stderr, "connect error: %s\n", strerror(errno));
			exit(1);
		}
		return 0;
	} else {
		print();
		return 1;
	}
}

#if defined(__linux__) || defined (__linux)
void
do_epoll()
{
	int epfd = epoll_create(1024);
	struct epoll_event events[20];

	while(1) {
		int sockfd;
		if((sockfd = socket(res -> ai_family, res -> ai_socktype, res -> ai_protocol)) == -1) {
	        fprintf(stderr, "socket error: %s\n", strerror(errno));
	        return;
	    }
		set_noblock(sockfd);
		struct epoll_event ev;
		ev.data.fd = sockfd;
		ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
		epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
		int ret = nonb_connect(sockfd);
		if(ret)
			goto gogogo;
		int nfds = epoll_wait(epfd, events, 20, -1);
		if(events[0].data.fd == sockfd) {
			print();
		}
gogogo:
		close(sockfd);
		epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, NULL);
	}
}
#elif defined(__APPLE__) && defined (__MACH__)
void
do_kqueue()
{
	int kq = kqueue();
	if(kq == -1) {
		perror("create kq error!");
		exit(1);
	}

	while(1) {
		int sockfd;
		if((sockfd = socket(res -> ai_family, res -> ai_socktype, res -> ai_protocol)) == -1) {
	        fprintf(stderr, "socket error: %s\n", strerror(errno));
	        return;
	    }
		set_noblock(sockfd);
		struct kevent changes[1];
		EV_SET(&changes[0], sockfd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
		kevent(kq, changes, 1, NULL, 0, NULL);
		int ret = nonb_connect(sockfd);
		if(ret)
			goto gogogo;
		struct kevent events[10];
		struct timespec timeout = {3, 0};
		int r = kevent(kq, NULL, 0, events, 10, &timeout);
		if(r == 0) {
			printf("timeout !\n");
		} else {
			print();
		}
gogogo:
		close(sockfd);
	}
}
#endif

int main(int argc, char **argv)
{
	if(argc != 3)
		usage(argv[0]);

	char *path = argv[1];
	char *port = argv[2];
	memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(path, port, &hints, &res) == -1) {
        fprintf(stderr, "不能解析地址\n");
        exit(1);
    }

    sprintf(pr_str, "TCPPING %s:%s -- ", path, port);
	#if defined(__linux__) || defined (__linux)
	do_epoll();
	#elif defined(__APPLE__) && defined (__MACH__)
	do_kqueue();
	#endif

}
