/* Autthor PRIYANSHU MORBAITA */

#include "alloc.h"

_Atomic bool allocator_initialized = false;
static pthread_once_t heap_init_once = PTHREAD_ONCE_INIT;

static const size_t size_classes[MAX_SIZE_CLASSES] = {
    16, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024,
    1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576, 32768, 49152
};

static heap global_mem = {0};

__thread tcache_t tcache = {0};
__thread bool tcache_initialized = false;


static struct {
    void *slab;
    size_t slab_size;
    size_t offset;
    meta_block *free_list;
    pthread_mutex_t lock;
} meta_allocator = {0};

#define HANDLE_ERROR(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void init_meta_allocator(void) {
    if (meta_allocator.slab) return;
    meta_allocator.slab = mmap(NULL, META_SLAB_SIZE, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (meta_allocator.slab == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed in init_meta_allocator: %s\n", strerror(errno));
        meta_allocator.slab = NULL;
        return;
    }
    meta_allocator.slab_size = META_SLAB_SIZE;
    meta_allocator.offset = 0;
    meta_allocator.free_list = NULL;
    if (pthread_mutex_init(&meta_allocator.lock, NULL) != 0) {
        HANDLE_ERROR("pthread_mutex_init failed in init_meta_allocator");
    }
}

static void *meta_alloc(const size_t size) {
    if (!meta_allocator.slab) {
        init_meta_allocator();
        if (!meta_allocator.slab) return NULL;
    }
    size_t aligned_size = (size + 7U) & ~7U;
    if (pthread_mutex_lock(&meta_allocator.lock) != 0) {
        HANDLE_ERROR("pthread_mutex_lock failed in meta_alloc");
    }
    meta_block *current = meta_allocator.free_list;
    meta_block *prev = NULL;
    while (current) {
        if (current->free && current->size >= aligned_size) {
            current->free = 0;
            if (current->size > aligned_size + sizeof(meta_block) + 32) {
                meta_block *new_block = (meta_block *)((char *)(current + 1) + aligned_size);
                new_block->size = current->size - aligned_size - sizeof(meta_block);
                new_block->free = 1;
                new_block->next = current->next;
                current->next = new_block;
                current->size = aligned_size;
            }
            if (pthread_mutex_unlock(&meta_allocator.lock) != 0) {
                HANDLE_ERROR("pthread_mutex_unlock failed in meta_alloc");
            }
            return (void *)(current + 1);
        }
        prev = current;
        current = current->next;
    }
    size_t total_size = sizeof(meta_block) + aligned_size;
    if (meta_allocator.offset + total_size > meta_allocator.slab_size) {
        fprintf(stderr, "Error: meta_alloc out of memory\n");
        pthread_mutex_unlock(&meta_allocator.lock);
        return NULL;
    }
    meta_block *new_block = (meta_block *)((char *)meta_allocator.slab + meta_allocator.offset);
    meta_allocator.offset += total_size;
    new_block->size = aligned_size;
    new_block->free = 0;
    new_block->next = NULL;
    if (prev) {
        prev->next = new_block;
    } else {
        meta_allocator.free_list = new_block;
    }
    if (pthread_mutex_unlock(&meta_allocator.lock) != 0) {
        HANDLE_ERROR("pthread_mutex_unlock failed in meta_alloc (end)");
    }
    return (void *)(new_block + 1);
}

static void *meta_calloc(const size_t num, const size_t size) {
    size_t total_size = num * size;
    void *ptr = meta_alloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

static void meta_free(void *ptr) {
    if (!ptr) return;
    meta_block *block = (meta_block *)((char *)ptr - sizeof(meta_block));
    if (pthread_mutex_lock(&meta_allocator.lock) != 0) {
        HANDLE_ERROR("pthread_mutex_lock failed in meta_free");
    }
    block->free = 1;
    if (block->next && block->next->free) {
        block->size += sizeof(meta_block) + block->next->size;
        block->next = block->next->next;
    }
    if (pthread_mutex_unlock(&meta_allocator.lock) != 0) {
        HANDLE_ERROR("pthread_mutex_unlock failed in meta_free");
    }
}

static inline size_t align_size(const size_t size) {
    return (size + ALIGNMENT - 1U) & ~(ALIGNMENT - 1U);
}

static inline int get_size_class(const size_t size) {
    for (int i = 0; i < MAX_SIZE_CLASSES; ++i) {
        if (size <= size_classes[i]) return i;
    }
    return -1;
}

static void init_tcache(void) {
    if (!tcache_initialized && atomic_load(&allocator_initialized)) {
        memset(&tcache, 0, sizeof(tcache));
        tcache_initialized = true;
    }
}

void init(void) {
    if (atomic_load(&allocator_initialized)) return;
    static volatile int init_lock = 0;
    while (__sync_lock_test_and_set(&init_lock, 1)) {}
    if (!atomic_load(&allocator_initialized)) {
        memset(&global_mem, 0, sizeof(global_mem));
        for (int i = 0; i < MAX_SIZE_CLASSES; i++) {
            global_mem.global_free_list[i] = meta_calloc(1, sizeof(Globally));
            if (global_mem.global_free_list[i]) {
                if (pthread_mutex_init(&global_mem.global_free_list[i]->lock, NULL) != 0) {
                    HANDLE_ERROR("pthread_mutex_init failed in init");
                }
                global_mem.global_free_list[i]->slabs = NULL;
                global_mem.global_free_list[i]->free_list = NULL;
            } else {
                fprintf(stderr, "Error: meta_calloc failed in init for size class %d\n", i);
            }
        }
        global_mem.large_free_list = NULL;
        global_mem.large_slabs = NULL;
        atomic_thread_fence(memory_order_seq_cst);
        atomic_store(&allocator_initialized, true);
    }
    __sync_lock_release(&init_lock);
}

static void *allocate_slab(size_t size) {
    if (size < (size_t)getpagesize()) {
        size = (size_t)getpagesize();
    }
    void *slab = mmap(NULL, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (slab == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed in allocate_slab: %s\n", strerror(errno));
        return NULL;
    }
    return slab;
}

static int populate_memory(const int sc_index) {
    if (sc_index < 0 || sc_index >= MAX_SIZE_CLASSES) {
        return 0;
    }
    size_t block_size = align_size(sizeof(block_header) + size_classes[sc_index]);
    void *slab = allocate_slab(SLAB_SIZE);
    if (!slab) return 0;
    int blocks_in_slab = (int)((SLAB_SIZE - sizeof(slab_node)) / block_size);
    if (blocks_in_slab <= 0) {
        munmap(slab, SLAB_SIZE);
        return 0;
    }
    slab_node *node = meta_alloc(sizeof(slab_node));
    if (!node) {
        munmap(slab, SLAB_SIZE);
        return 0;
    }
    node->slab = slab;
    node->size = SLAB_SIZE;
    node->next = global_mem.global_free_list[sc_index]->slabs;
    global_mem.global_free_list[sc_index]->slabs = node;
    block_header *new_free_list = (block_header *)slab;
    for (int i = 0; i < blocks_in_slab; i++) {
        block_header *blk = (block_header *)((char *)slab + i * block_size);
        blk->size_class = sc_index;
        blk->freed = false;
        blk->magic = BLOCK_MAGIC;
        blk->next = (i == blocks_in_slab - 1) ? NULL :
                    (block_header *)((char *)slab + (i + 1) * block_size);
    }
    block_header *tail = new_free_list;
    while (tail->next) tail = tail->next;
    tail->next = global_mem.global_free_list[sc_index]->free_list;
    global_mem.global_free_list[sc_index]->free_list = new_free_list;
    return 1;
}

static void *large_alloc(const size_t size) {
    size_t total_size = align_size(size + sizeof(large_block));
    void *slab = allocate_slab(total_size);
    if (!slab) return NULL;
    slab_node *node = meta_alloc(sizeof(slab_node));
    if (!node) {
        munmap(slab, total_size);
        return NULL;
    }
    node->slab = slab;
    node->size = total_size;
    node->next = global_mem.large_slabs;
    global_mem.large_slabs = node;
    large_block *block = (large_block *)slab;
    block->size = size;
    block->free = 0;
    block->freed = false;
    block->magic = LARGE_MAGIC;
    block->next = NULL;
    return (void *)(block + 1);
}

void *my_alloc(const size_t size) {
    if (size == 0) return NULL;
    if (!atomic_load(&allocator_initialized)) {
        init();
        if (!atomic_load(&allocator_initialized)) return NULL;
    }
    if (!tcache_initialized) {
        init_tcache();
    }
    if (size >= LARGE_BLOCK_THRESHOLD) {
        return large_alloc(size);
    }
    int sc_index = get_size_class(size);
    if (sc_index == -1) return NULL;
    if (tcache.cache[sc_index].cache_count > 0) {
        block_header *block = tcache.cache[sc_index].cache_list[--tcache.cache[sc_index].cache_count];
        if (block && block->magic == BLOCK_MAGIC) {
            block->freed = false;
            return (void *)(block + 1);
        }
    }
    block_header *block = NULL;
    Globally *global_list = global_mem.global_free_list[sc_index];
    if (global_list && pthread_mutex_trylock(&global_list->lock) == 0) {
        block_header *free_list = global_list->free_list;
        if (free_list) {
            block = free_list;
            global_list->free_list = block->next;
        }
        pthread_mutex_unlock(&global_list->lock);
    }
    if (!block && global_list) {
        if (pthread_mutex_lock(&global_list->lock) == 0) {
            block_header *free_list = global_list->free_list;
            if (free_list) {
                block = free_list;
                global_list->free_list = block->next;
            } else if (populate_memory(sc_index)) {
                free_list = global_list->free_list;
                if (free_list) {
                    block = free_list;
                    global_list->free_list = block->next;
                }
            }
            pthread_mutex_unlock(&global_list->lock);
        }
    }
    if (!block) return NULL;
    block->freed = false;
    block->next = NULL;
    return (void *)(block + 1);
}

void my_free(void *ptr) {
    if (!ptr || !atomic_load(&allocator_initialized)) return;
    if (!tcache_initialized) {
        init_tcache();
    }
    large_block *large = (large_block *)((char *)ptr - sizeof(large_block));
    if (large->magic == LARGE_MAGIC && !large->freed) {
        large->freed = true;
        large->free = 1;
        return;
    }
    block_header *block = (block_header *)((char *)ptr - sizeof(block_header));
    if (block->magic != BLOCK_MAGIC || block->freed) {
        return;
    }
    int sc_index = block->size_class;
    if (sc_index < 0 || sc_index >= MAX_SIZE_CLASSES) {
        return;
    }
    block->freed = true;
    block->next = NULL;
    if (tcache.cache[sc_index].cache_count < CACHE_SIZE) {
        tcache.cache[sc_index].cache_list[tcache.cache[sc_index].cache_count++] = block;
        return;
    }
    Globally *global_list = global_mem.global_free_list[sc_index];
    if (global_list && pthread_mutex_trylock(&global_list->lock) == 0) {
        int flush_count = CACHE_SIZE / 2;
        for (int i = 0; i < flush_count; i++) {
            block_header *cached_block = tcache.cache[sc_index].cache_list[--tcache.cache[sc_index].cache_count];
            cached_block->next = global_list->free_list;
            global_list->free_list = cached_block;
        }
        pthread_mutex_unlock(&global_list->lock);
        tcache.cache[sc_index].cache_list[tcache.cache[sc_index].cache_count++] = block;
    }
}

void print_allocator_status(void) {
    printf("=== Allocator Status ===\n");
    if (!atomic_load(&allocator_initialized)) {
        printf("Allocator not initialized\n");
        return;
    }
    for (int i = 0; i < MAX_SIZE_CLASSES; i++) {
        if (!global_mem.global_free_list[i]) continue;
        int free_count = 0;
        block_header *blk = global_mem.global_free_list[i]->free_list;
        while (blk && free_count < 1000) {
            free_count++;
            blk = blk->next;
        }
        int tcache_count = tcache_initialized ? tcache.cache[i].cache_count : 0;
        if (free_count > 0 || tcache_count > 0) {
            printf("Size Class %zu: %d free blocks, %d in tcache\n",
                   size_classes[i], free_count, tcache_count);
        }
    }
    printf("=========================\n");
}

void thread_cache_cleanup(void) {
    if (!tcache_initialized) return;
    for (int i = 0; i < MAX_SIZE_CLASSES; i++) {
        Globally *global_list = global_mem.global_free_list[i];
        if (global_list && tcache.cache[i].cache_count > 0) {
            if (pthread_mutex_lock(&global_list->lock) == 0) {
                for (int j = 0; j < tcache.cache[i].cache_count; j++) {
                    block_header *block = tcache.cache[i].cache_list[j];
                    if (block) {
                        block->next = global_list->free_list;
                        global_list->free_list = block;
                    }
                }
                tcache.cache[i].cache_count = 0;
                pthread_mutex_unlock(&global_list->lock);
            }
        }
    }
    tcache_initialized = false;
}

void allocator_cleanup(void) {
    if (!atomic_load(&allocator_initialized)) return;
    thread_cache_cleanup();
    for (int i = 0; i < MAX_SIZE_CLASSES; i++) {
        if (global_mem.global_free_list[i]) {
            slab_node *slab = global_mem.global_free_list[i]->slabs;
            while (slab) {
                slab_node *next = slab->next;
                if (slab->slab) {
                    munmap(slab->slab, slab->size);
                }
                meta_free(slab);
                slab = next;
            }
            pthread_mutex_destroy(&global_mem.global_free_list[i]->lock);
            meta_free(global_mem.global_free_list[i]);
        }
    }
    slab_node *large_slab = global_mem.large_slabs;
    while (large_slab) {
        slab_node *next = large_slab->next;
        if (large_slab->slab) {
            large_block *block = (large_block *)large_slab->slab;
            size_t size = block->size;
            munmap(large_slab->slab, align_size(size + sizeof(large_block)));
        }
        meta_free(large_slab);
        large_slab = next;
    }
    if (meta_allocator.slab) {
        pthread_mutex_destroy(&meta_allocator.lock);
        munmap(meta_allocator.slab, META_SLAB_SIZE);
        meta_allocator.slab = NULL;
    }
    atomic_store(&allocator_initialized, false);
}
