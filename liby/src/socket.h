//only for linux

#ifndef LIBY_SOCKET_T
#define LIBY_SOCKET_T

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <linux/tcp.h>

#include <errno.h>

#define QLEN 10

void set_noblock(int fd);
void set_nonagle(int fd);
int initsocket(int type, const struct sockaddr *addr, socklen_t alen, int qlen);
int initserver(const char *server_name, const char *bind_port);


#endif
