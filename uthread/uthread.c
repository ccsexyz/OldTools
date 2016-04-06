#include "uthread.h"

void uthread_push(schedule_t *sch, uthread_t *p) {
    if (p == NULL || sch == NULL)
        return;
    if (sch->uthreads_head == NULL) {
        sch->uthreads_head = p;
        sch->uthreads_tail = sch->uthreads_head;
        p->next = p->prev = NULL;
    } else {
        sch->uthreads_tail->next = p;
        p->prev = sch->uthreads_tail;
        p->next = NULL;
        sch->uthreads_tail = p;
    }
}

uthread_t *uthread_pop(schedule_t *sch) {
    uthread_t *tail;
    if (sch == NULL || (tail = sch->uthreads_tail) == NULL || tail == NULL)
        return NULL;

    if (tail->prev == tail) {
        sch->uthreads_head = sch->uthreads_tail = NULL;
    } else {
        tail->prev->next = tail->next;
        tail->next->prev = tail->prev;
        sch->uthreads_tail = tail->prev;
    }

    return tail;
}

void uthread_del(schedule_t *sch, uthread_t *p) {
    if (p == NULL || sch == NULL || sch->uthreads_head == NULL)
        return;

    uthread_t *head = sch->uthreads_head;
    while (head) {
        if (head == p) {
            if (sch->uthreads_head == head->next) {
                uthread_pop(sch);
                return;
            }

            uthread_t *tail = sch->uthreads_tail;
            sch->uthreads_tail = p;
            uthread_pop(sch);
            sch->uthreads_tail = tail;
            return;
        }
    }
    head = head->next;
}

uthread_t *uthread_create(schedule_t *s, uthread_func func, void *arg) {
    uthread_t *u = (uthread_t *)malloc(sizeof(uthread_t));
    if (u == NULL)
        return NULL;

    u->next = u->prev = NULL;
    u->arg = arg;
    u->func = func;
    u->state = READY;
    u->stack_size = DEFAULT_STACK_SIZE;
    u->stack = (char *)malloc(u->stack_size);
    if (u->stack == NULL) {
        free(u);
        return NULL;
    }

    uthread_push(s, u);

    return u;
}

int schedule_finished(schedule_t *s) {
    if (s->running_uthread != NULL) {
        return 0;
    } else {
        uthread_t *p = s->uthreads_head;
        while (p) {
            if (p->state != FREE) {
                return 0;
            }
            p = p->next;
        }
    }

    return 1;
}

void uthread_resume(schedule_t *s, uthread_t *u) {
    if (u == NULL)
        return;

    switch (u->state) {
    case READY:
        getcontext(&(u->context));

        u->context.uc_stack.ss_sp = u->stack;
        u->context.uc_stack.ss_size = DEFAULT_STACK_SIZE;
        u->context.uc_stack.ss_flags = 0;
        u->context.uc_link = &(s->main);
        u->state = RUNNING;

        s->running_uthread = u;

        makecontext(&(u->context), (void (*)(void))(uthread_body), 1, s);

    case SUSPEND:

        u->state = RUNNING;
        s->running_uthread = u;
        swapcontext(&(s->main), &(u->context));
        break;

    default:;
    }
}

void uthread_yield(schedule_t *s) {
    if (s->running_uthread != NULL) {
        uthread_t *u = s->running_uthread;
        u->state = SUSPEND;
        s->running_uthread = NULL;

        swapcontext(&(u->context), &(s->main));
    }
}

void uthread_body(schedule_t *s) {
    uthread_t *u = s->running_uthread;
    if (u != NULL) {
        u->func(u->arg);
        u->state = FREE;
        s->running_uthread = NULL;
    }
}
