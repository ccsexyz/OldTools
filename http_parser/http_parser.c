#include "http_parser.h"

static void *
safe_malloc(size_t n)
{
    void *ret = malloc(n);

    if(ret == NULL) {
        fprintf(stderr, "malloc error: %s\n", strerror(errno));
        exit(1);
    }

    return ret;
}

static char
parse_hex(const char *s)
{
    int n1, n2;
    if(*s >= '0' && *s <= '9')
        n1 = *s - '0';
    else {
        char temp = *s | 32;
        n1 = temp - 'a' + 10;
    }
    if(*(s + 1) >= '0' && *(s + 1) <= '9')
        n2 = *(s + 1) - '0';
    else {
        char temp = *(s + 1) | 32;
        n2 = temp - 'a' + 10;
    }
    return (n1 * 16 + n2);
}

static char *
convert_query_string(const char *src)
{
    assert(src);
    size_t n = strlen(src);
    char *ret = safe_malloc(n + 1);
    int i = 0, j = 0;
    for(i = 0, j = 0; i < n; i++, j++) {
        if(*(src + i) == '%') {
            *(ret + j) = parse_hex(src + i + 1);
            i += 2;
        } else {
            *(ret + j) = *(src + i);
        }
    }
    *(ret + j) = '\0';
    return ret;
}

request_t *
http_request_init()
{
    request_t *ret = safe_malloc(sizeof(request_t));
    memset(ret, 0, sizeof(request_t));
    return ret;
}

void
http_request_destroy(request_t *r)
{
    if(r) {
        REQUEST_FREE(r->method_string);
        REQUEST_FREE(r->path_string);
        REQUEST_FREE(r->query_string);
        REQUEST_FREE(r->version_string);
        REQUEST_FREE(r->data);

        while(r->head) {
            head_line *next = r->head->next;
            REQUEST_FREE(r->head->key);
            REQUEST_FREE(r->head->value);
            free(r->head);
            r->head = next;
        }

        free(r);
    }
}

void
parse_request_line(request_t *r, char *line)
{
    assert(r && line);

    for(int i = 0; line; i++) {
        char *temp = strsep(&line, " ");

        switch(i) {
        case 0:
            //printf("request method: %s\n", temp);
            r->method_string = strdup(temp);
            break;
        case 1:
            //printf("request uri: %s\n", temp);
            r->query_string = index(temp, '?');
            if(r->query_string) {
                *(r->query_string) = '\0';
                r->query_string = convert_query_string(r->query_string + 1);
            }

            *temp = 0;
            r->path_string = strdup(temp + 1);
            break;
        case 2:
            //printf("http version: %s\n", temp);
            r->version_string = strdup(temp);
            break;
        }
    }
}

static void
http_request_head_push(request_t *r, char *key, char *value)
{
    assert(r && key && value);
    head_line *line = safe_malloc(sizeof(head_line));
    line->key = strdup(key);
    line->value = strdup(value);
    line->next = NULL;

    if(r->tail) {
        r->tail->next = line;
        r->tail = line;
    } else {
        r->head = r->tail = line;
    }
}

static void
parse_head_line(request_t *r, char *line)
{
    assert(r && line);

    char *temp = strsep(&line, ":");
    if(temp && line) {
        //printf("first: %s\nsecond: %s\n", temp, line);
        http_request_head_push(r, temp, line);
    }
}

static void
parse_data(request_t *r, char *data)
{
    assert(r && data);
    if(*data) r->data = strdup(data);
    //puts(data);
}

void
print_http_request(request_t *r)
{
    assert(r);

    if(r->method_string) printf("method: %s\n", r->method_string);
    if(r->path_string) printf("path: %s\n", r->path_string);
    if(r->query_string) printf("query: %s\n", r->query_string);
    if(r->version_string) printf("version: %s\n", r->version_string);

    head_line *head = r->head;
    while(head) {
        if(head->key && head->value) {
            printf("%s: %s\n", head->key, head->value);
        }
        head = head->next;
    }

    if(r->data) printf("data: %s\n", r->data);
}

request_t *
http_request_parse(const char *src)
{
    char *str = strdup(src);

    request_t *r = http_request_init();

    for(int i = 0; str; i++) {
        char *temp = strsep(&str, "\r\n");
        if(str) {
            *(str++) = 0;
        }
        //printf("%p %p: %s\n", temp, str, temp);
        if(i == 0) {
            parse_request_line(r, temp);
        } else if(str == NULL) {
            parse_data(r, temp);
        } else {
            parse_head_line(r, temp);
        }
    }

    free(str);
    return r;
}
