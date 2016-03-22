//
// Created by ccsexyz on 16-3-20.
//

#include "test.h"
#include "DeferCaller_test.h"
#include "FileHandle_test.h"
#include "Sinal_test.h"
#include "Timestamp_test.h"

void test() {
    run_test(Timestamp_test(), DeferCaller_test(), Signal_test(),
             FileHandle_test());
}
