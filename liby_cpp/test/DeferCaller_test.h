//
// Created by ccsexyz on 16-3-20.
//

#ifndef LIBY_DEFERCALLER_TEST_H
#define LIBY_DEFERCALLER_TEST_H

#include "../util.h"
#include "test.h"
#include <iostream>

class DeferCaller_test {
public:
    void operator()() {
        DeferCaller call_later(
            [] { std::cout << "DeferCaller_test pass" << std::endl; });
    }
};

#endif // LIBY_DEFERCALLER_TEST_H
