#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "http_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define CLRF "\r\n"
#define REQUEST_FREE(x) do{if(x) free(x);}while(0)

typedef struct head_line_{
    char *key;
    char *value;
    struct head_line_ *next;
} head_line;

typedef struct {
    char *method_string;
    char *path_string;
    char *query_string;
    char *version_string;
    head_line *head;
    head_line *tail;
    char *data;
} request_t;

request_t *http_request_init();

void http_request_destroy(request_t *r);

void print_http_request(request_t *r);

request_t *http_request_parse(const char *src);


#endif // HTTP_PARSER_H
