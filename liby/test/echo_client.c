#include "../src/liby.h"

void output_handler(liby_client *client, char *buf, off_t length, int ec) {
    if (ec != 0) {
        log_quit("error!");
    } else {
        write(1, buf, length);
    }
}

void input_handler(liby_client *input_client, char *buf, off_t length, int ec) {
    if (ec != 0) {
        // error
        log_quit("input error");
    } else {
        liby_client **p;
        int n = get_clients_of_loop(input_client->loop, &p);
        log_info("n = %d\n", n);
        for(int i = 0; i < n; i++) {
            liby_async_write_some(p[i], buf, length, output_handler);
        }
        free(p);
        //liby_async_write_some(client, buf, length, output_handler);
        liby_async_read(input_client, input_handler);
    }
}

void conn(liby_client *client) {
    // printf("connect success!\n");
    // getchar();
}

int main(int argc, char **argv) {
    int fd = liby_connect_tcp(argv[1], argv[2]);
    if (fd < 0) log_quit("connect error!");

    liby_loop *loop = liby_create_easy();
    assert(loop);

    liby_client *client = liby_client_init(fd, loop);
    liby_client *input_client = liby_client_init(0, loop);

    liby_async_read(input_client, input_handler);
    set_connect_handler_for_client(client, conn);

    run_liby_main_loop(loop);

    return 0;
}
