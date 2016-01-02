#include <stdio.h>
#include "liby.h"

int main(int argc, char *argv[])
{
    char *str = strdup("h22e22l22l22o22w22o22r22l22d");
    char **ret = liby_split1(str, "22");
    liby_split_print(ret);
    liby_split_free(ret);
    free(str);
    return 0;
}
