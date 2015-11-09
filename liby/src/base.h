#include <pthread.h>

typedef void (*mutex_func)(mutex_t *);
typedef void (*cond_func)(cond_t *);

typedef struct {
    pthread_mutex_t mutex;
    mutex_func lock;
    mutex_func unlock;
} mutex_t;

typedef struct {
    pthread_cond_t cond;
    mutex_t *m;
    cond_func signal;
    cond_func broadcast;
    cond_func wait;
} cond_t;

void
mutex_init(mutex_t *m)
{
    pthread_mutex_init(&(m->mutex), NULL);
    m->lock = mutex_lock;
    m->unlock = mutex_unlock;
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
    c->signal = cond_signal;
    c->wait = cond_wait;
    c->broadcast = cond_broadcast;
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
    pthread_cond_wait(&(c->cond), c->m);
}

void
cond_broadcast(cond_t *c)
{
    pthread_cond_broadcast(&(c->cond));
}
