#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//00->end 01->1 10->0 11->end

#define BUFFER_LENGTH (1024)
int non_zero_nums;

unsigned char key_buf[4096];
int keybuf_off;

typedef struct item_ {
    unsigned char num;          //原始数据，长度为一个字节
    int ntimes;                 //此数据在文件中出现的次数
    int flag;                   //
    char result_string[20];     //哈弗曼编码的字符串表示
    int result_string_length;   //哈弗曼的字符串长度
    unsigned char result[4];    //哈弗曼编码结果在文件中表示，
                                //注意，在文件中时，一位的哈弗曼编码将被表示成两位,
                                //01 -> 1 10 -> 0 00 or 11 -> end [end表示哈弗曼编码结束] [不足一个字节的补0]
                                //这样主要是为了将哈弗曼编码保存到目标文件时以一种可扩展的方式保存而不用维护其长度字段
                                //举例: 1000的哈弗曼编码将表示为 0110101000 补0后为 0110101000000000 长度为2个字节
    int result_length;          //哈弗曼编码字节数，1 - 4

    struct item_ *left, *right; //哈弗曼树的左右子节点
} item_t;

typedef struct num_key_ {
    unsigned char num;          //原始数据
    unsigned char result_key[0];//处理后的哈弗曼编码，每两个位表示哈弗曼编码一个位
} num_key_t;

typedef struct head_ {
    char magic_word[8];         //压缩文件魔术字，”ccsexyz”
    long file_size;             //压缩前的文件长度
    int non_zero_nums;          //在原文件中出现次数不为0的字节数，即被编码的字节数
} head_t;

void
usage(char *name)
{
    printf("usage: %s <filepath>!\n", name);
}

FILE *
safe_fopen(const char *restrict filename, const char *restrict mode)
{
    if(filename == NULL || mode == NULL)
        goto err;
    FILE *fp = fopen(filename, mode);
    if(fp != NULL)
        return fp;
err:
    fprintf(stderr, "cannot open the file %s!\n", filename);
    exit(1);
}

long
get_file_size(FILE *f)
{
    long filesize;
    long oldoffset;
    oldoffset = ftell(f);
    fseek(f, 0, SEEK_END);
    filesize = ftell(f);
    fseek(f, oldoffset, SEEK_SET);
    return filesize;
}

void *
safe_malloc(long int n)
{
    if(n < 0)
        goto err;
    void *p = malloc(n);
    if(p != NULL)
        return p;
err:
    puts("malloc error!\n");
    exit(1);
}

void
count_numbers(void **buf_ptr, item_t **ret_ptr, size_t buf_size)
{
    if(buf_ptr == NULL || ret_ptr == NULL || buf_size == 0) exit(1);

    unsigned char *buf = (unsigned char *)(*buf_ptr);
    item_t *ret = *ret_ptr;

    for(int i = 0; i < buf_size; i++) {
        ret[buf[i]].ntimes++;
    }
}

void
print_result(item_t **ret_ptr)
{
    for(int i = 0; i < 256; i++)
        printf("[%3d]: num = %-3d; times = %d\n", i, (*ret_ptr)[i].num, (*ret_ptr)[i].ntimes);
}

char result_string[20];

void
print_huffman(item_t **ret_ptr)
{
//    printf("1\n");
    item_t *ret = *ret_ptr;
//    printf("2\n");
    if(ret->flag == 0) {
//        printf("3\n");
        printf("num = %d, ntimes = %d, result_string = %s\n", ret->num, ret->ntimes, result_string);
    } else {
        //printf("num = %d ret->left = %p, ret->right = %p\n", ret->num, ret->left, ret->right);
        sprintf(result_string, "%s1", result_string);
        print_huffman(&ret->left);
        result_string[strlen(result_string) - 1] = 0;
        sprintf(result_string, "%s0", result_string);
        print_huffman(&ret->right);
        result_string[strlen(result_string) - 1] = 0;
    }
}

void
do_huffman(item_t **ret_ptr, item_t **items_ptr)
{
    item_t *ret = *ret_ptr;
    item_t *items = *items_ptr;
    if(ret->flag == 0) {
        sprintf(ret->result_string, "%s", result_string);
        ret->result_string_length = strlen(ret->result_string);
        ret->result_length = ret->result_string_length / 4 + 1;
        for(int i = 0; i < ret->result_length; i++) {
            for(int j = 0; j < 8; j += 2) {
                int temp = i * 4 + j / 2;
                if(temp >= ret->result_string_length) {
                    break;
                }
                if(ret->result_string[temp] == '1') {
                    ret->result[i] += 1 << (6 - j);
                } else if(ret->result_string[temp] == '0') {
                    ret->result[i] += 2 << (6 - j);
                }
            }
        }

        num_key_t key;
        key.num = ret->num;

        memcpy(key_buf + keybuf_off, &key, sizeof(key));
        keybuf_off += sizeof(key);
        memcpy(key_buf + keybuf_off, ret->result, sizeof(unsigned char) * ret->result_length);
        keybuf_off += ret->result_length;

        memcpy(&(items[ret->num]), ret, sizeof(item_t));
        //free(ret);
        non_zero_nums++;
    } else {
        sprintf(result_string, "%s1", result_string);
        do_huffman(&ret->left, items_ptr);
        result_string[strlen(result_string) - 1] = 0;
        sprintf(result_string, "%s0", result_string);
        do_huffman(&ret->right, items_ptr);
        result_string[strlen(result_string) - 1] = 0;
    }
}

int
numsort(const void *p1, const void *p2)
{
    //return *(int *)p1 - *(int *)p2;
    return ((item_t *)p2)->ntimes - ((item_t *)p1)->ntimes;
}

unsigned char
str_to_unum(unsigned char *c, size_t n)
{
    if(n == 1) {
        return *c == '1' ? 1 : 0;
    } else {
        unsigned char num = str_to_unum(c + 1, n - 1);
        unsigned char ret = ((*c == '1') << (n - 1)) + num;
        //printf("ret = %x\n", ret);
        return ret;
    }
}

int main(int argc, char **argv)
{
    if(argc != 2) {
        usage(argv[0]);
        exit(1);
    }

    FILE *fp = safe_fopen(argv[1], "r+");
    long file_size = get_file_size(fp);

    if(file_size == 0) {
        puts("file_size == 0!\n");
        sleep(1);
        exit(1);
    }

    void *buf = safe_malloc(file_size);
    size_t ret = fread(buf, 1, file_size, fp);

    if(ret == 0 && ferror(fp)) {
        printf("error read!\n");
        free(buf);
        exit(1);
    } else {
        fclose(fp);
    }

    item_t *items = (item_t *)safe_malloc(sizeof(item_t) * 256);
    memset((void *)items, 0, sizeof(item_t) * 256);
    for(int i = 0; i < 256; i++) {
        items[i].num = i;
    }

    count_numbers(&buf, &items, file_size);

    int one_character_flag = 0;
    if(items[0].ntimes == file_size)
        one_character_flag = 1;

    qsort((void *)items, 256, sizeof(item_t), numsort);
    //print_result(&items);

    int length = 256;
    for(int i = length - 1; i > 0; i--) {
        if(items[i].ntimes == 0) {
            continue;
        }

        item_t *left = (item_t *)safe_malloc(sizeof(item_t));
        item_t *right = (item_t *)safe_malloc(sizeof(item_t));
        memcpy((void *)left, (void *)(&items[i - 1]), sizeof(item_t));
        memcpy((void *)right, (void *)(&items[i]), sizeof(item_t));

        items[i - 1].flag = 1;
        items[i - 1].ntimes = left->ntimes + right->ntimes;
        items[i - 1].left = left;
        items[i - 1].right = right;

        qsort((void *)items, i, sizeof(item_t), numsort);
    }

    item_t *head_item = (item_t *)safe_malloc(sizeof(item_t));
    memcpy(head_item, items, sizeof(item_t));
    //print_result(&items);
    print_huffman(&head_item);

    if(!one_character_flag) {
        do_huffman(&head_item, &items);
    }
    else {
        printf("only one character!\n");
        sprintf(items[head_item->num].result_string, "1");
        items[head_item->num].result_string_length = 1;
        items[head_item->num].result_length = 1;
        items[head_item->num].result[0] = 1 << 6;
        memcpy(key_buf, &(items[head_item->num].num), sizeof(unsigned char));
        memcpy(key_buf + sizeof(unsigned char), &(items[head_item->num].result[0]), sizeof(unsigned char));
        keybuf_off = sizeof(unsigned char) * 2;
        non_zero_nums = 1;
    }

    char *new_file_path = (char *)safe_malloc(strlen(argv[1]) + 10);
    sprintf(new_file_path, "%s.compress", argv[1]);
    FILE *new_fp;
    if((new_fp = fopen(new_file_path, "w+")) == NULL) {
        fprintf(stderr, "create new file error!\n");
        exit(2);
    }

    head_t head;
    head.file_size = file_size;
    head.non_zero_nums = non_zero_nums;
    sprintf(head.magic_word, "ccsexyz");

    fwrite(&head, 1, sizeof(head), new_fp);
    fwrite(key_buf, 1, keybuf_off, new_fp);

    printf("file_size = %ld\nhead size = %ld\non_zero_nums = %d\n", file_size, sizeof(head) + keybuf_off, non_zero_nums);

    unsigned char data_buf[BUFFER_LENGTH];
    char string_buf[BUFFER_LENGTH * 8];
    int strbuf_off = 0;

    //printf("wtf!\n");
    for(int i = 0; i < file_size; i++) {
        item_t *p = &items[((unsigned char *)buf)[i]];

        if(strbuf_off + p->result_string_length >= BUFFER_LENGTH * 8) {
            //printf("will write sth!\n");
            int temp = BUFFER_LENGTH * 8 - strbuf_off;
            memcpy(string_buf + strbuf_off, p->result_string, temp);
            for(int j = 0; j < BUFFER_LENGTH; j++)
                data_buf[j] = str_to_unum(string_buf + 8 * j, 8);
            fwrite((void *)data_buf, 1, BUFFER_LENGTH, new_fp);
            strbuf_off = p->result_string_length - temp;
            if(strbuf_off != 0) {
                memcpy(string_buf, p->result_string + temp, strbuf_off);
            }
        } else {
            //printf("strbuf_off = %d\np->result_string_length = %d\n", strbuf_off, p->result_string_length);
            memcpy(string_buf + strbuf_off, p->result_string, p->result_string_length);
            strbuf_off += p->result_string_length;


            if(i == file_size - 1) {
                int data_buf_off = strbuf_off / 8 + (strbuf_off % 8 != 0);
                for(int j = 0; j < data_buf_off; j++)
                    data_buf[j] = str_to_unum(string_buf + 8 * j, 8);
                fwrite((void *)data_buf, 1, data_buf_off, new_fp);
            }
        }
    }

    printf("key_buf[0].num = %d\nkey_buf[0].key = %d\n", key_buf[0], key_buf[1]);
finish:
    fclose(new_fp);
    free(buf);
    free(items);
    free(new_file_path);
    free(head_item);
    return 0;
}
