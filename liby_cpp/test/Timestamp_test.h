//
// Created by ccsexyz on 16-3-20.
//

#ifndef LIBY_TIMESTAMP_TEST_H
#define LIBY_TIMESTAMP_TEST_H

#include "../util.h"
#include "test.h"
#include <iostream>
#include <unistd.h>

class Timestamp_test {
public:
    void operator()() {
        DeferCaller defer(
            [] { std::cout << "Timestamp_test pass!" << std::endl; });
        run_test(test_constructer, test_compare_func, test_toString,
                 test_invalidTime);
    }

private:
    static void test_constructer() {
        Timestamp t1;
        Timestamp t2(123);
        Timestamp t3(12, 12);

        assert(t1.invalid());
        assert(t2.valid());
        assert(t3.valid());
    }

    static void test_compare_func() {
        auto t1 = Timestamp::now();
        usleep(100);
        auto t2 = Timestamp::now();
        auto t3 = t2;

        assert(t3 == t2);
        assert(t1 < t2);
        assert(t1 < t3);
        assert(t2 > t1);
        assert(t3 > t1);
    }

    static void test_toString() {
        auto t1 = Timestamp::now();
        assert(t1.valid());
        auto str = t1.toString();
        assert(!str.empty());
    }

    static void test_invalidTime() {
        auto t1 = Timestamp::invalidTime();
        assert(t1.invalid());
    }
};

#endif // LIBY_TIMESTAMP_TEST_H
