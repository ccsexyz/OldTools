#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ysplit.h"

int main(int argc, char *argv[])
{
    char *str = strdup("h22e22l22l22o22w22o22r22l22d");
    ysplit_print(ysplit1(str, "22"));
    free(str);
    return 0;
}
