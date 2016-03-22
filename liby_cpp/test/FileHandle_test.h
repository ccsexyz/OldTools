//
// Created by JohnsonJohn on 16/3/21.
//

#ifndef LIBY_FILEHANDLE_TEST_H
#define LIBY_FILEHANDLE_TEST_H

#include "../FileHandle.h"
#include "../util.h"
#include "test.h"
#include <iostream>

class FileHandle_test {
public:
    void operator()() { run_test(Basic_test, AddFd_test); }

private:
    static void Basic_test() {
        FileHandle FH1 = 13;
        FileHandle FH2;
        assert(FH1.valid());
        assert(FH2.invalid());
        FH2.setfd(12);
        assert(FH2.valid());
        FH2.resetfd();
        assert(FH2.invalid());
    }
    static void AddFd_test() { ; }
};

#endif // LIBY_FILEHANDLE_TEST_H
