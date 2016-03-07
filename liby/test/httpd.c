#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include "liby.h"

liby_server *httpd_server;

char *filename(char *buf, off_t *filesize) {
    char *method = strsep(&buf, " ");
    char *path = strsep(&buf, " ");

    if (path == NULL || buf == NULL) return NULL;

    path = strdup(path + 1);
    struct stat file;

    if (stat(path, &file)) {
        free(path);
        return NULL;
    }

    *filesize = file.st_size;
    return path;
}

void write_header_handler(liby_client *client, char *buf, off_t length,
                          int ec) {
    if (!ec) {
        off_t *data = (off_t *)(client->data);
        off_t offset = 0;
        sendfile(client->sockfd, data[0], &offset, data[1]);
        free(buf);
        close(data[0]);
    }

    del_client_from_server(client, client->server);
}

void read_all_handler(liby_client *client, char *buf, off_t length, int ec) {
    if (ec) {
        fprintf(stderr, "%s\n", "error!");
    } else {
        off_t *data = safe_malloc(sizeof(off_t) * 2);
        char *path = filename(buf, &data[1]);
        if (path) {
            sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");
            size_t length = strlen(buf);

            data[0] = open(path, O_RDONLY, 0);
            free(path);
            if (data[0] >= 0) {
                set_data_of_client_with_free(client, data);
                liby_async_write_some(client, buf, length,
                                      write_header_handler);
                return;
            }
        }
    }

    del_client_from_server(client, client->server);
}

void httpd_aceptor(liby_client *client) {
    if (client == NULL) printf("client is NULL");
    liby_async_read(client, read_all_handler);
}

int main(int argc, char **argv) {
    httpd_server = liby_server_init("localhost", "9377");
    if (httpd_server == NULL) exit(1);

    epoller_t *loop = epoller_init(10240);
    if (loop == NULL) exit(2);

    set_acceptor_for_server(httpd_server, httpd_aceptor);

    add_server_to_epoller(httpd_server, loop);

    run_epoll_main_loop(loop, get_default_epoll_handler());

    liby_server_destroy(httpd_server);
    epoller_destroy(loop);

    return 0;
}
