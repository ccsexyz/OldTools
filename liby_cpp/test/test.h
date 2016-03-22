//
// Created by ccsexyz on 16-3-20.
//

#ifndef LIBY_TEST_H
#define LIBY_TEST_H

#include <assert.h>

template <typename T> void run_test(T v) { v(); }

template <typename T, typename... N> void run_test(T v, N... n) {
    v();
    run_test(n...);
}

void test();

#endif // LIBY_TEST_H
