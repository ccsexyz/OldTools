#ifndef SOCKET_H
#define SOCKET_H

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
int connect_tcp(const char *host, const char *port);
int connect_tcp_noblock(const char *host, const char *port);

#endif // SOCKET_H
