#include "base.h"

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
