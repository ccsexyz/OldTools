#include "base.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void
mutex_init(mutex_t *m)
{
    pthread_mutex_init(&(m->mutex), NULL);
}

void
mutex_destroy(mutex_t *m)
{
    pthread_mutex_destroy(&(m->mutex));
}

void
mutex_lock(mutex_t *m)
{
    pthread_mutex_lock(&(m->mutex));
}

void
mutex_unlock(mutex_t *m)
{
    pthread_mutex_unlock(&(m->mutex));
}

void
cond_init(cond_t *c)
{
    pthread_cond_init(&(c->cond), NULL);
}

void
cond_destroy(cond_t *c)
{
    pthread_cond_destroy(&(c->cond));
}

void
cond_signal(cond_t *c)
{
    pthread_cond_signal(&(c->cond));
}

void
cond_wait(cond_t *c)
{
    pthread_cond_wait(&(c->cond), &(c->m->mutex));
}

void
cond_broadcast(cond_t *c)
{
    pthread_cond_broadcast(&(c->cond));
}

char **
liby_split1(char *src, const char *sp)
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

char **
liby_split(const char *src, const char *sp)
{
    char *str_copy = strdup(src);
    char **ret = liby_split1(str_copy, sp);
    if(str_copy) free(str_copy);
    return ret;
}

char **
liby_split_free(char **p)
{
    if(p) {
        char **p1 = p;
        while(*p) {
            free(*p++);
        }
        free(p1);
    }
}

char **
liby_split_print(char **p)
{
    if(p) {
        while(*p) {
            puts(*p++);
        }
    }
}
