#ifndef _UTHREAD_H
#define _UTHREAD_H

#include <stdio.h>
#include <ucontext.h>

#define DEFAULT_STACK_SIZE (1024 * 1024)
enum UThreadState { FREE, READY, RUNNING, SUSPEND };

typedef void (*uthread_func)(void *arg);
// typedef void (*scheduler) (schedule_t *sch);

typedef struct uthread_t {
    ucontext_t context;
    uthread_func func;
    void *arg;
    enum UThreadState state;
    char *stack;
    size_t stack_size;
    struct uthread_t *next;
    struct uthread_t *prev;
} uthread_t;

typedef struct schedule_t {
    // ucontext_t scheduler_context;
    ucontext_t main;
    int uthread_num;
    uthread_t *running_uthread;
    uthread_t *uthreads_head;
    uthread_t *uthreads_tail;
} schedule_t;

void uthread_push(schedule_t *sch, uthread_t *p);
void uthread_del(schedule_t *sch, uthread_t *p);
void uthread_resume(schedule_t *s, uthread_t *u);
void uthread_yield(schedule_t *s);
void uthread_body(schedule_t *s);

int schedule_finished(schedule_t *s);

uthread_t *uthread_pop(schedule_t *sch);
uthread_t *uthread_create(schedule_t *s, uthread_func func, void *arg);

#endif
