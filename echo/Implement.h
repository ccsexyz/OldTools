//
// Created by JohnsonJohn on 15/9/24.
//

#ifndef ECHO_IMPLEMENT_H
#define ECHO_IMPLEMENT_H

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>


class Implement {
    const static int QLEN = 10;
    const static int HOST_NAME_MAX = 256;
    const int MAXLINE = 4096;
public:
    static void set_noblock(int fd);
    static void set_nonagle(int fd);
    static int initserver(const char *server_name, const char *bind_port);
    static int initsocket(int type, const struct sockaddr *addr, socklen_t alen, int qlen);
};


#endif //ECHO_IMPLEMENT_H
