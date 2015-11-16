#include "liby/liby.h"

int main(int argc, char **argv)
{
    int fd = liby_sync_connect_tcp(argv[1], argv[2]);
    if(fd < 0)
        log_quit("connect error!");

    epoller_t *loop = epoller_init(10240);
    if(loop == NULL)
        log_quit("create epoller error!");

    liby_client *client = liby_client_init(fd, loop);
}
