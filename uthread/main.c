#include "uthread.h"
#include <stdio.h>
#include <unistd.h>

void func2(void *arg) {
    for (int i = 0; i < 90; i++) {
        printf("i2 = %d\n", i);
        fflush(stdout);
        uthread_yield((schedule_t *)arg);
    }
}

void func3(void *arg) {
    for (int i = 0; i < 9; i++) {
        printf("i3 = %d\n", i);
        fflush(stdout);
        uthread_yield((schedule_t *)arg);
    }
}

void schedule_test() {
    schedule_t s;

    uthread_t *u1 = uthread_create(&s, func3, &s);
    uthread_t *u2 = uthread_create(&s, func2, &s);

    while (!schedule_finished(&s)) {
        uthread_resume(&s, u1);
        uthread_resume(&s, u2);
    }

    puts("over");
}

int main(void) { schedule_test(); }
