#include "ysplit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **ysplit1(char *src, const char *sp)
{
    int n = 8;
    char **ret = malloc(sizeof(char *) * n);
    memset(ret, 0, sizeof(char *) * n);

    int i = 0;
    char *s1 = src;
    int nsp = strlen(sp) - 1;

    while(s1) {
        char *temp = strsep(&s1, sp);

        if(s1) s1 += nsp;
        if(i == n - 2) {
            n = n * 2;
            ret = realloc(ret, sizeof(char *) * n);
        }

        ret[i++] = strdup(temp);
        //printf("i = %d string = %s\n", i, s1);
    }
    ret[i] = NULL;

    return ret;
}

char **ysplit(const char *src, const char *sp)
{
    char *str_copy = strdup(src);
    char **ret = ysplit1(str_copy, sp);
    if(str_copy) free(str_copy);
    return ret;
}

char **ysplit_free(char **p)
{
    if(p) {
        char **p1 = p;
        while(*p) {
            free(*p++);
        }
        free(p1);
    }
}

char **ysplit_print(char **p)
{
    if(p) {
        while(*p) {
            puts(*p++);
        }
    }
}
