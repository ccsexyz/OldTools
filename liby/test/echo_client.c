#include "liby.h"

//暂时还只能用全局变量

void
input_handler(liby_client *input_client, char *buf, off_t length, int ec)
{
    if(ec) {
        //error
        log_quit("input error");
    } else {
        liby_client *client =  (liby_client *)get_data_of_client(input_client);
        liby_async_write_some(client, buf, length, NULL);
    }
}

int main(int argc, char **argv)
{
    int fd = liby_sync_connect_tcp(argv[1], argv[2]);
    if(fd < 0)
        log_quit("connect error!");

    epoller_t *loop = epoller_init(10240);
    if(loop == NULL)
        log_quit("create epoller error!");

    set_noblock(0);

    liby_client *client = liby_client_init(fd, loop);
    liby_client *input_client = liby_client_init(0, loop);
    set_data_of_client_without_free(input_client, (void *)client);
    liby_async_read(input_client, input_handler);

    run_epoll_main_loop(loop, get_default_epoll_handler());

    return 0;
}
