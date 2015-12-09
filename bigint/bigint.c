#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAGIC_NUM 9
#define MAX_NUM 1000000000

typedef struct {
    char *num_str;
    int *num;
    int length;
    int size; //size = length / 9 + (length % 9) != 0
} bigint_t;

bigint_t *
bigint_init(const char *num_str)
{
    int i = 0;
    for(i = 0; num_str[i]; i++) {
        if(num_str[i] > '9' || num_str[i] < '0')
            return NULL;
    }

    bigint_t *b = (bigint_t *)malloc(sizeof(bigint_t));
    char temp[MAGIC_NUM + 1];

    if(b) {
        b->num_str = strdup(num_str);
        b->length = i;
        b->size = b->length / 9 + (b->length % 9 != 0);
        b->num = (int *)malloc(sizeof(int) * b->size);

        int length = b->length;
        int size = b->size;
        int *num = b->num;

        for(i = 0; i < size; i++) {
            int tail = length - MAGIC_NUM * i - 1;
            int head = length - MAGIC_NUM * (i + 1);
            if(head < 0)
                head = 0;

            int j = 0;
            for(j = 0; j < (tail - head + 1); j++) {
                temp[j] = num_str[head + j];
            }
            temp[j] = 0;

            int result = atoi(temp);
            num[size - 1 - i] = result;
        }
    }

    return b;
}

int
bigint_equal(bigint_t *a, bigint_t *b)
{
    if(!a || !b || a->size != b->size) {
        return 0;
    } else {
        for(int i = 0; i < a->size; i++) {
            if(a->num[i] != b->num[i])
                return 0;
        }
        return 1;
    }
}

// 1 -> a > b
// 0 -> a == b
//-1 -> a < b
//-2 -> error
int
bigint_cmp(bigint_t *a, bigint_t *b)
{
    if(!a || !b) {
        return -2;
    } else if(a->size > b->size) {
        return 1;
    } else if(a->size < b->size) {
        return -1;
    } else {
        for(int i = 0; i < a->size; i++) {
            int sub = a->num[i] - b->num[i];
            if(sub > 0)
                return 1;
            else if(sub < 0)
                return -1;
        }
        return 0;
    }
}

bigint_t *
bigint_add(bigint_t *a, bigint_t *b)
{
    if(a == NULL || b == NULL)
        return NULL;

    //默认a的宽度大于b
    if(a->size < b->size)
        return bigint_add(b, a);

    int size = b->size;
    int *num1 = a->num;
    int *num2 = b->num;

    bigint_t *n = (bigint_t *)malloc(sizeof(bigint_t));
    n->num = (int *)malloc(sizeof(int) * (a->size + 1));
    n->size = a->size;
    int *num3 = n->num;

    int flag = 0;
    for(int i = 0; i < size; i++) {
        int j1 = a->size - 1 - i;
        int j2 = size - 1 - i;
        int result = num1[j1] + num2[j2] + flag;
        if(result > 1000000000) {
            flag = 1;
            result -= 1000000000;
        }
        num3[j1] = result;
    }
    if(flag == 1) {
        memmove(num3, num3 + 1, sizeof(int) * a->size);
        num3[0] = 1;
        n->size++;
    }
    char *num_str = (char *)malloc(sizeof(char) * MAGIC_NUM * n->size);
    num_str[0] = 0;
    size = n->size;
    for(int i = 0; i < size; i++) {
        if(i == 0)
            sprintf(num_str, "%s%d", num_str, num3[i]);
        else
            sprintf(num_str, "%s%09d", num_str, num3[i]);
    }
    n->length = strlen(num_str);
    n->num_str = num_str;

    return n;
}

// a >= b
// if a < b error
bigint_t *
bigint_sub(bigint_t *a, bigint_t *b)
{
    int cmp = bigint_cmp(a, b);
    if(cmp == -2 || cmp == -1) {
        return NULL;
    } else if(cmp == 0) {
        return bigint_init("0");
    }

    int size1 = a->size;
    int size2 = b->size;
    int *num1 = a->num;
    int *num2 = b->num;
    int *num = (int *)malloc(sizeof(int) * size1);
    int size = size1;
    memcpy(num, num1, sizeof(int) * size1);


    for(int i = 0; i < size2; i++) {
        int result = num[size1 - 1 - i] - num2[size2 - 1 - i];
        printf("result = %d\n", result);
        if(result >= 0) {
            num[size1 - 1 - i] = result;
        } else {
            result += MAX_NUM;
            num[size1 - 1 - i] = result;
            num[size1 - 1 - i - 1]--;
        }
    }

    for(int i = 0; i < size1; i++) {
        printf("num[%d] = %d\n", i, num[i]);
        if(num[i]) {
            size = size1 - i;
            printf("i = %d size1 = %d\n", i, size1);
            break;
        }
    }
    if(size != size1) {
        memmove(num, num + (size1 - size), size);
    }

    char *num_str = (char *)malloc(sizeof(char) * size * MAGIC_NUM);
    num_str[0] = 0;
    for(int i = 0; i < size; i++) {
        if(i == 0)
            sprintf(num_str, "%s%d", num_str, num[i]);
        else
            sprintf(num_str, "%s%09d", num_str, num[i]);
    }

    bigint_t *n = (bigint_t *)malloc(sizeof(bigint_t));
    n->size = size;
    n->length = strlen(num_str);
    n->num = num;
    n->num_str = num_str;

    return n;
}

void
bigint_release(void *p)
{
    if(p) {
        bigint_t *b = p;
        if(b->num) {
            free(b->num);
        }
        if(b->num_str) {
            free(b->num_str);
        }
    }
}

void
bigint_free(void *p)
{
    if(p) {
        bigint_release(p);
        free(p);
    }
}

bigint_t *
bigint_multiply(bigint_t *a, bigint_t *b)
{
    // a * b
    int *num1 = a->num;
    int *num2 = b->num;
    int size1 = a->size;
    int size2 = b->size;
    int size = size1 + size2;
    int *num = (int *)malloc(sizeof(int) * (size1 + size2));
    for(int i = 0; i < size; i++) {
        num[i] = 0;
    }
    for(int i = 0; i < size2; i++) {
        for(int j = 0; j < size1; j++) {
            long n1 = num1[size1 - 1 - j];
            long n2 = num2[size2 - 1 - i];
            long result = n1 * n2;
            //printf("result = %ld\n", result);
            long adder = result / MAX_NUM;
            result %= MAX_NUM;
            int curr = size - 1 - (i + j);
            num[curr] += result;
            if(num[curr] > MAX_NUM) {
                num[curr] -= MAX_NUM;
                adder++;
            }
            //printf("adder = %ld result = %ld curr = %d\n", adder, result, curr);
            while(1) {
                curr--;
                //printf("curr = %d\n", curr);
                num[curr] += adder;
                if(num[curr] < MAX_NUM) {
                    break;
                } else {
                    num[curr] -= MAX_NUM;
                    adder = 1;
                }
            }
        }
    }
    int zeros = 0;
    for(int i = 0; i < size; i++) {
        if(num[i]) {
            zeros = i;
            break;
        }
    }
    memmove(num, num + zeros, (size - zeros) * sizeof(int));
    size -= zeros;
    //printf("size = %d zeros = %d\n", size, zeros);
    char *num_str = (char *)malloc(sizeof(char) * size * MAGIC_NUM);
    memset(num_str, 0, sizeof(char) * size * MAGIC_NUM);
    for(int i = 0; i < size; i++) {
        if(i == 0)
            sprintf(num_str, "%s%d", num_str, num[i]);
        else
            sprintf(num_str, "%s%09d", num_str, num[i]);
    }
    bigint_t *nb = (bigint_t *)malloc(sizeof(bigint_t));
    nb->num_str = num_str;
    nb->length = strlen(num_str);
    nb->num = num;
    nb->size = size;
    return nb;
}

bigint_t *
bigint_pow_iter(bigint_t *b, int times, int *flag, bigint_t *solve)
{
    if(flag[times - 1]) {
        return &(solve[times - 1]);
    } else {
        bigint_t *b1 = bigint_pow_iter(b, times / 2, flag, solve);
        if(!flag[times / 2 - 1]) {
            flag[times / 2 - 1] = 1;
            solve[times / 2 - 1] = *b1;
            free(b1);
            b1 = solve + times / 2 - 1;
        }
        bigint_t *b2 = bigint_pow_iter(b, times - times / 2, flag, solve);
        if(!flag[times - times / 2 - 1]) {
            flag[times - times / 2 - 1] = 1;
            solve[times - times / 2 - 1] = *b2;
            free(b2);
            b2 = solve + (times - times / 2 - 1);
        }
        bigint_t *b3 = bigint_multiply(b1, b2);
        return b3;
    }
}

bigint_t *
bigint_pow(bigint_t *b, int times)
{
    int *flag = (int *)malloc(sizeof(int) * times);
    bigint_t *solve = (bigint_t *)malloc(sizeof(bigint_t) * times);
    memset(flag, 0, sizeof(int) * times);

    flag[0] = 1;
    solve[0] = *b;

    bigint_t *ret = bigint_pow_iter(b, times, flag, solve);

    for(int i = 0; i < times; i++) {
        if(flag[i]) {
            bigint_release(&solve[i]);
        }
    }

    free(flag);
    free(solve);

    return ret;
}

char itoa_buffer[40];

const char *
my_itoa(int number)
{
    sprintf(itoa_buffer, "%d", number);
    return itoa_buffer;
}

void
bigint_print(bigint_t *b)
{
    if(b)
        printf("%s\n", b->num_str);
    else
        printf("error!\n");
}

// a must be bigger than b
bigint_t *
bigint_div(bigint_t *a, bigint_t *b)
{
    if(!a || !b) {
        return NULL;
    }

    int cmp = bigint_cmp(a, b);
    if(cmp == 0) {
        return bigint_init("1");
    } else if(cmp < 0) {
        return bigint_init("0");
    }

    bigint_t *ret;
    int *num1 = a->num;
    int *num2 = b->num;
    int size1 = a->size;
    int size2 = b->size;
    int size = size1 - size2 + 1;
    int *num = (int *)malloc(sizeof(int) * size);
    char *num_str = (char *)malloc(sizeof(char) * size * MAGIC_NUM);
    num_str[0] = 0;

    if(size2 == 1) {
        long b_num = num2[0];
        long adder = 0;
        for(int i = 0; i < size1; i++) {
            long a_num = num1[i] + adder * MAX_NUM;
            if(a_num > b_num) {
                num[i] = a_num / b_num;
                adder = a_num % b_num;
            } else if(a_num == b_num) {
                num[i] = 1;
                adder = 0;
            } else {
                num[i] = 0;
                adder = a_num;
            }
        }
        int zeros = 0;
        for(int i = 0; i < size; i++) {
            if(num[i]) {
                zeros = i;
                break;
            }
        }
        memmove(num, num + zeros, (size - zeros) * sizeof(int));
        size -= zeros;
        for(int i = 0; i < size; i++) {
            if(i == 0)
                sprintf(num_str, "%s%d", num_str, num[i]);
            else
                sprintf(num_str, "%s%09d", num_str, num[i]);
        }
        ret = (bigint_t *)malloc(sizeof(bigint_t));
        ret->length = strlen(num_str);
        ret->num = num;
        ret->num_str = num_str;
        ret->size = size;
        return ret;
    } else {
        ;
    }

    if(size1 == size2) {
        int div1 = num1[0] / num2[0] + 1;   //max
        int div2 = num1[0] / (num2[0] + 1); //min
        //printf("div1 = %d\n", div1);
        //printf("div2 = %d\n", div2);
        int result = 0;
        for(int i = div2; i <= div1; i++) {
            bigint_t *guess = bigint_init(my_itoa(i));
            bigint_t *m = bigint_multiply(b, guess);
            bigint_print(m);
            int cmp = bigint_cmp(a, m);
            bigint_print(guess);
            //printf("i = %d\n", cmp);
            bigint_free(guess);
            if(cmp == -2) {
                return NULL;
            } else if(cmp == -1) {
                result = i - 1;
                break;
            } else if(cmp == 0) {
                result = i;
                break;
            }
        }

        return bigint_init(my_itoa(result));
    } else if(size1 == size2 + 1) {
        int div1 = num1
    }

    char *str = strdup(a->num_str);
    str[b->length] = 0;
    bigint_t *t1 = bigint_init(str);
    free(str);
    bigint_t *d1 = bigint_div()

    return NULL;
}

int main(int argc, char **argv)
{
    bigint_t *n1 = bigint_init("9000000000000000000000000000000000000000000");
    bigint_t *n2 = bigint_init("999999999999999999999999999999999999999999");
    //bigint_print(bigint_pow(n1, 8999));
    bigint_print(bigint_div(n1, n2));
    //bigint_print(bigint_multiply(n2, bigint_init("1")));
    return 0;
}
