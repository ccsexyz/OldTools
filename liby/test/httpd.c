#include "../src/liby.h"
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

char *filename(char *buf, off_t *filesize) {
    char *method = strsep(&buf, " ");
    char *path = strsep(&buf, " ");

    if (path == NULL || buf == NULL)
        return NULL;

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
    off_t *data = (off_t *)(client->data);
    off_t offset = 0;

    if (ec > 0) {
        log_err("httpd");
    } else if (ec < 0) {
        log_info("client quit int the write header handler");
    } else {
        sendfile(client->sockfd, data[0], &offset, data[1]);
    }

    close(data[0]);
    free(buf);
}

void read_all_handler(liby_client *client, char *buf, off_t length, int ec) {
    if (ec > 0) {
        log_err("httpd");
    } else if (ec < 0) {
        log_info("client quit in the read_all_handler");
    } else {
        off_t *data = safe_malloc(sizeof(off_t) * 2);
        char *path = filename(buf, &data[1]);
        if (path) {
            sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");
            size_t length = strlen(buf);

            data[0] = open(path, O_RDONLY, 0);
            free(path);
            if (data[0] >= 0) {
                liby_client_set_data_with_free(client, data);
                liby_async_write_some(client, buf, length,
                                      write_header_handler);
                return;
            }
        } else {
            free(data);
            log_info("http request parse error!");
        }
    }
    free(buf);
    liby_destroy_client_later(client);
}

void httpd_aceptor(liby_client *client) {
    assert(client);
    liby_async_read(client, read_all_handler);
}

int main(int argc, char **argv) {
    liby_server *httpd_server =
        liby_server_init("0.0.0.0", "8000", httpd_aceptor);
    liby_loop *loop = liby_create_easy();
    add_server_to_loop(httpd_server, loop);
    run_liby_main_loop(loop);
    return 0;
}
