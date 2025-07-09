/* Autthor PRIYANSHU MORBAITA */

#ifndef ALLOC_H
#define ALLOC_H

#define _GNU_SOURCE
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#define true 1
#define false 0

// Configuration constants
#define ALIGNMENT 16
#define SLAB_SIZE (64 * 1024) // 64KB slabs
#define MAX_SIZE_CLASSES 23
#define CACHE_SIZE 32
#define LARGE_BLOCK_THRESHOLD 65536
#define META_SLAB_SIZE (64 * 1024) // 64KB for metadata allocation

// Magic numbers
#define BLOCK_MAGIC 0xDEADBEEF
#define LARGE_MAGIC 0xFEEDFACE


typedef struct meta_block {
    size_t size;
    int free;
    struct meta_block *next;
} meta_block;

// Block header structure
typedef struct block_header {
    int size_class;
    bool freed;
    uint32_t magic;
    struct block_header *next;
} block_header;

// Slab node structure
typedef struct slab_node {
    void *slab;
    size_t size;
    struct slab_node *next;
} slab_node;

// Large block structure
typedef struct large_block {
    size_t size;
    int free;
    bool freed;
    struct large_block *next;
    uint32_t magic;
} large_block;

// Per-size-class cache for thread-local storage
typedef struct cache_entry {
    block_header *cache_list[CACHE_SIZE];
    int cache_count;
} cache_entry;

// Thread cache structure
typedef struct tcache_t {
    cache_entry cache[MAX_SIZE_CLASSES];
} tcache_t;

// Global free list for each size class
typedef struct Globally {
    pthread_mutex_t lock;
    slab_node *slabs;
    block_header *free_list;
} Globally;

// Global heap structure
typedef struct heap {
    Globally *global_free_list[MAX_SIZE_CLASSES];
    large_block *large_free_list;
    slab_node *large_slabs;
} heap;

// Function declarations
void *my_alloc(size_t size);
void my_free(void *ptr);
void init(void);
void allocator_cleanup(void);
void thread_cache_cleanup(void);
void print_allocator_status(void);

#endif // ALLOC_H
