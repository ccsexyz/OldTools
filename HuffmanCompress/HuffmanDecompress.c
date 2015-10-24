#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//00->end 01->1 10->0 11->end

#define BUFFER_LENGTH (1024)

typedef struct item_ {
    unsigned char num;
    int flag;

    struct item_ *left, *right;
} item_t;

typedef struct num_key_ {
    unsigned char num;
    unsigned char result_key[0];
} num_key_t;

typedef struct head_ {
    char magic_word[8];
    long file_size;
    int non_zero_nums;
} head_t;


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

item_t *
alloc_new_item(void)
{
    item_t *temp = (item_t *)safe_malloc(sizeof(item_t));
    memset(temp, 0, sizeof(item_t));
    return temp;
}

int
key_trans_iter(unsigned char n)
{
    if(n == 2)
        return 0;
    else if(n == 1)
        return 1;
    else if(n == 0 || n == 3)
        return 2;
    else
        return -1;
}

char result_string[200];

void
huffman_build(item_t **root_ptr, void **keybuf_ptr, int non_zero_nums)
{
    if(root_ptr == NULL || keybuf_ptr == NULL) {
        fprintf(stderr, "root or key_buf cannot be NULL!\n");
        exit(1);
    }

    void *key_buf = *keybuf_ptr;
    if(*root_ptr == NULL) {
        *root_ptr = alloc_new_item();
    }
    item_t *root = *root_ptr;

    item_t *curr = root;

    int off = 0;
    while(non_zero_nums) {
        num_key_t *kptr = (num_key_t *)(key_buf + off);
        printf("num = %d\n", kptr->num);
        off += sizeof(num_key_t);
        unsigned char *result_key = kptr->result_key;
        off++;

        for(int i = 0; ; i++) {
            if(i % 4 == 0 && i != 0) {
                i = 0;
                result_key++;
                off++;
            }

            int step = key_trans_iter(((*result_key) >> (6 - 2 * i)) & 3);

            if(step == -1) {
                fprintf(stderr, "wrong step!\n");
                exit(1);
            } else if(step == 0) {
                if(curr->right == NULL) {
                    curr->right = alloc_new_item();
                }
                curr = curr->right;
                sprintf(result_string, "%s0", result_string);
            } else if(step == 1) {
                if(curr->left == NULL) {
                    curr->left = alloc_new_item();
                }
                curr = curr->left;
                sprintf(result_string, "%s1", result_string);
            } else if(step == 2) {
                curr->flag = 1;
                curr->num = kptr->num;
                printf("build %c %d %s\n", curr->num, curr->num, result_string);
                result_string[0] = 0;
                non_zero_nums--;
                break;
            }
        }

        curr = root;
        printf("off = %d\n", off);
    }

    *(unsigned char **)keybuf_ptr = (unsigned char *)key_buf + off;
}

void
huffman_destroy(item_t **root_ptr)
{
    item_t *root = *root_ptr;

    if(root->left)
        huffman_destroy(&(root->left));
    if(root->right)
        huffman_destroy(&(root->right));

    *root_ptr = NULL;
    free(root);
}

void
print_huffman(item_t **ret_ptr)
{
//    printf("1\n");
    item_t *ret = *ret_ptr;
//    printf("2\n");
    if(ret->flag == 1) {
//        printf("3\n");
        printf("num = %d, result_string = %s\n", ret->num, result_string);
    } else {
        printf("num = %d ret->left = %p, ret->right = %p\n", ret->num, ret->left, ret->right);
        if(ret->left != NULL) {
            sprintf(result_string, "%s1", result_string);
            print_huffman(&ret->left);
            result_string[strlen(result_string) - 1] = 0;
        }
        if(ret->right != NULL) {
            sprintf(result_string, "%s0", result_string);
            print_huffman(&ret->right);
            result_string[strlen(result_string) - 1] = 0;
        }
    }
}

int main(int argc, char **argv)
{
    if(argc != 2) {
        fprintf(stderr, "usage: %s <filepath>!\n", argv[0]);
        exit(1);
    }

    FILE *fp = safe_fopen(argv[1], "r+");
    long file_size = get_file_size(fp);

    if(file_size == 0) {
        fprintf(stderr, "empty file!\n");
        sleep(1);
        exit(1);
    }

    void *buf = safe_malloc(file_size);
    size_t ret = fread(buf, 1, file_size, fp);
    printf("read %d bytes!\n", ret);

    if(ret == 0 && ferror(fp)) {
        fprintf(stderr, "error read!\n");
        free(buf);
        exit(1);
    } else {
        fclose(fp);
    }

    char *file = (char *)safe_malloc(strlen(argv[1]) + strlen(".decompress") + 1);
    sprintf(file, "%s.decompress", argv[1]);
    fp = safe_fopen(file, "w+");

    head_t *head = (head_t *)buf;
    if(strcmp("ccsexyz", head->magic_word)) {
        fprintf(stderr, "wrong magic words!\n");
        printf("magic word = %s\n", head->magic_word);
        exit(1);
    }

    printf("non_zero_nums = %d\n", head->non_zero_nums);

    item_t *root = NULL;
    unsigned char *vbuf = (unsigned char *)(buf) + sizeof(head_t);
    huffman_build(&root, (void **)(&vbuf), head->non_zero_nums);
    print_huffman(&root);

    long new_file_size = head->file_size;
    unsigned char *wbuf = (unsigned char *)safe_malloc(new_file_size * sizeof(unsigned char));

    item_t *curr = root;
    int vbuf_iter = 0;
    for(long i = 0; i < new_file_size; ) {
        unsigned char value = vbuf[vbuf_iter];
        //printf("value = %c %d\n", value, value);
        for(int j = 0; j < 8; j++) {
            int rc = (value >> (7 - j)) & 1;

            if(rc) {
                curr = curr->left;
            } else {
                curr = curr->right;
            }

            if(curr->flag) {
                //printf("write %c %d\n", curr->num, curr->num);
                wbuf[i] = curr->num;
                i++;
                curr = root;
            }
        }
        vbuf_iter++;
    }

    ret = fwrite(wbuf, sizeof(unsigned char), new_file_size, fp);
    printf("write %d bytes!\nthe number should be %ld bytes!\n", ret, new_file_size);

finish:
    fclose(fp);
    free(file);
    free(buf);
    free(wbuf);
    huffman_destroy(&root);
}
