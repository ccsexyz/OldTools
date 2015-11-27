/*
 * 闲的蛋疼写的测试数据类型长度的程序
 */

#include <stdio.h>

#define TEST_LENGTH(X) do{ printf("size = %ld\n", sizeof(X)); } while(0)

int main(void)
{
    TEST_LENGTH(char);
    TEST_LENGTH(short);
    TEST_LENGTH(int);
    TEST_LENGTH(long);
    TEST_LENGTH(long long);
    TEST_LENGTH(float);
    TEST_LENGTH(double);
    TEST_LENGTH(long double);
    return 0;
}

/* 测试结果
 * 1 2 4 8 8 4 8 16
 * 测试环境：mac os x 10.11.1 64bit
 */
