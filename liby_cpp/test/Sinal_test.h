//
// Created by JohnsonJohn on 16/3/21.
//

#ifndef LIBY_SINAL_TEST_H
#define LIBY_SINAL_TEST_H

#include "../util.h"
#include "test.h"
#include <iostream>
#include <signal.h>

class Signal_test {
public:
    void operator()() {
        DeferCaller call_later(
            [] { std::cout << "Signal_test pass" << std::endl; });
        run_test(Signal_catch_test);
    }

private:
    static void Signal_catch_test() {
        int num = 0;
        Signal::signal(SIGUSR1, [&num] { num = 1; });
        ::raise(SIGUSR1);
        int i = 0;
        while (num == 0) {
            i++;
            usleep(100);
            if (i > 10) {
                assert(0);
            }
        }
    }
    static void Signal_reset_test() {
        int num = 0;
        Signal::signal(SIGUSR1, [&num] { num = 1; });
        Signal::reset(SIGUSR1);
        ::raise(SIGUSR1);
        usleep(100);
        assert(num != 1);
    }
};

#endif // LIBY_SINAL_TEST_H
