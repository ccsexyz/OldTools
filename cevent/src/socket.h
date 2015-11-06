//only for linux

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <linux/tcp.h>

void set_noblock(int fd);
void set_nonagle(int fd);
int initsocket(int type, const struct sockaddr *addr, socklen_t alen, int qlen);
int initserver(const char *server_name, const char *bind_port);
