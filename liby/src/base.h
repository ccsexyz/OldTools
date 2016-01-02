#ifndef LIBY_BASE_H
#define LIBY_BASE_H

#include <pthread.h>

#define author "yxx"
#define LOCK(x) do {mutex_lock(&(x));}while(0)
#define UNLOCK(x) do {mutex_unlock(&(x));}while(0)
#define SIGNAL(x) do {cond_signal(&(x));}while(0)
#define BROADCAST(x) do {cond_broadcast(&(x));}while(0)
#define WAIT(x) do {cond_wait(&(x));}while(0)

typedef struct {
    pthread_mutex_t mutex;
} mutex_t;

typedef struct {
    pthread_cond_t cond;
    mutex_t *m;
} cond_t;

typedef void (*mutex_func)(mutex_t *);
typedef void (*cond_func)(cond_t *);

void mutex_init(mutex_t *m);

void mutex_destroy(mutex_t *m);

void mutex_lock(mutex_t *m);

void mutex_unlock(mutex_t *m);

void cond_init(cond_t *c);

void cond_destroy(cond_t *c);

void cond_signal(cond_t *c);

void cond_wait(cond_t *c);

void cond_broadcast(cond_t *c);

char **liby_split1(char *src, const char *sp);

char **liby_split(const char *src, const char *sp);

char **liby_split_free(char **p);

char **liby_split_print(char **p);

#endif
