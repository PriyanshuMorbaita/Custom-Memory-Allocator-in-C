/* Autthor PRIYANSHU MORBAITA
   DATE :- 02-06-2025   
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

#define public __attribute__((visibility("default")))
#define private static
#define packed __attribute__((__packed__))
#define unused __attribute__((__unused__))
#define MAX_WORDS ((1024*1024*1024/4)-1)
#define ZERO_WORDS 1073741823

#define BLOCKSIZE 4
#define ErrNoErr    0
#define ERR_NO_MEM    1
#define ERR_UNKNOWN  2
#define ERR_DOUBLE_FREE   4

#define bool _Bool
#define true 1
#define false 0


typedef unsigned char int8;
typedef unsigned short int int16;
typedef unsigned int int32;
typedef unsigned long long int int64;
typedef void heap;
typedef int32 word;

struct packed s_header {
    word w:30;
    bool alloced:1;
    bool unused reserved:1;
};
typedef struct packed s_header header;

#define RET_ERROR(x) do { \
    errno = (x); \
    return NULL; \
} while(false)

#define findblock(x) findblock_($h memspace,(x),0)
#define show() show_($h memspace)
#define allock(x) alloc((x)*1024)
#define allocm(x) alloc((x)*(1024*1024))
#define allocg(x) allocm((x)*1024)

public bool destroy(void*);
private void show_(header*);
private header *findblock_(header*,word,word);
private void *mkalloc(word,header*);
public void *alloc(int32);
int main(int,char**);