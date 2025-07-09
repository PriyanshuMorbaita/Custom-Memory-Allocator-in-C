/* PRIYANSHU MORBAITA */


#include "alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define NUM_ALLOCS 10000
#define NUM_THREADS 5

size_t sizes[] = {32, 64, 128, 512, 1024, 2048};
#define NUM_SIZES (sizeof(sizes) / sizeof(sizes[0]))

double now_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void benchmark_custom() {
    void *blocks[NUM_ALLOCS] = {0}; 
    int alloc_count = 0;
    
    double start = now_sec();
    
    for (int i = 0; i < NUM_ALLOCS; i++) {
        size_t size = sizes[i % NUM_SIZES];
        blocks[i] = my_alloc(size);
        if (blocks[i]) {
            alloc_count++;
            memset(blocks[i], 0xAA, size);
        } else {
            break; 
        }
    }
    
    for (int i = 0; i < NUM_ALLOCS; i++) {
        if (blocks[i]) {
            my_free(blocks[i]);
            blocks[i] = NULL;
        }
    }
    
    double end = now_sec();
    printf("[Custom Allocator] Time: %.6f seconds, Allocated: %d/%d\n", 
           end - start, alloc_count, NUM_ALLOCS);
}

void benchmark_malloc() {
    void *blocks[NUM_ALLOCS] = {0}; 
    int alloc_count = 0;
    
    double start = now_sec();
    
    for (int i = 0; i < NUM_ALLOCS; i++) {
        size_t size = sizes[i % NUM_SIZES];
        blocks[i] = malloc(size);
        if (blocks[i]) {
            alloc_count++;
            memset(blocks[i], 0xAA, size);
        } else {
            break; 
        }
    }
    
    for (int i = 0; i < NUM_ALLOCS; i++) {
        if (blocks[i]) {
            free(blocks[i]);
            blocks[i] = NULL;
        }
    }
    
    double end = now_sec();
    printf("[Standard malloc/free] Time: %.6f seconds, Allocated: %d/%d\n", 
           end - start, alloc_count, NUM_ALLOCS);
}

void *thread_alloc(void *arg) {
    int thread_id = *((int*)arg);
    
    for (int i = 0; i < NUM_ALLOCS / NUM_THREADS; i++) {
        size_t size = sizes[i % NUM_SIZES];
        void *ptr = my_alloc(size);
        if (ptr) {
            memset(ptr, 0xBB, size);
            my_free(ptr);
        }
    }
    
    thread_cache_cleanup();
    return NULL;
}

void *thread_malloc(void *arg) {
    int thread_id = *((int*)arg);
    
    for (int i = 0; i < NUM_ALLOCS / NUM_THREADS; i++) {
        size_t size = sizes[i % NUM_SIZES];
        void *ptr = malloc(size);
        if (ptr) {
            memset(ptr, 0xBB, size);
            free(ptr);
        }
    }
    
    return NULL;
}

void benchmark_custom_multithreaded() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    double start = now_sec();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, thread_alloc, &thread_ids[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            return;
        }
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Failed to join thread %d\n", i);
        }
    }
    
    double end = now_sec();
    printf("[Custom Allocator Multithreaded] Time: %.6f seconds\n", end - start);
}

void benchmark_malloc_multithreaded() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    double start = now_sec();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, thread_malloc, &thread_ids[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            return;
        }
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Failed to join thread %d\n", i);
        }
    }
    
    double end = now_sec();
    printf("[Standard malloc/free Multithreaded] Time: %.6f seconds\n", end - start);
}

void stress_test() {
    printf("\n=== Stress Test ===\n");
    
    void *ptrs[1000];
    int allocated = 0;
    
    double start = now_sec();
    
    for (int round = 0; round < 100; round++) {
        for (int i = allocated; i < allocated + 50 && i < 1000; i++) {
            size_t size = sizes[rand() % NUM_SIZES];
            ptrs[i] = my_alloc(size);
            if (ptrs[i]) {
                memset(ptrs[i], 0xCC, size);
            }
        }
        allocated += 50;
        if (allocated > 1000) allocated = 1000;
        
        for (int i = 0; i < 25 && allocated > 0; i++) {
            int idx = rand() % allocated;
            if (ptrs[idx]) {
                my_free(ptrs[idx]);
                ptrs[idx] = ptrs[allocated - 1];
                ptrs[allocated - 1] = NULL;
                allocated--;
            }
        }
    }
    
    for (int i = 0; i < allocated; i++) {
        if (ptrs[i]) {
            my_free(ptrs[i]);
        }
    }
    
    double end = now_sec();
    printf("[Custom Allocator Stress Test] Time: %.6f seconds\n", end - start);
}

void benchmark_large_allocs() {
    printf("\n=== Large Allocation Benchmark ===\n");

    size_t large_sizes[] = {65536, 131072, 262144, 524288, 1048576};
    int num_large_sizes = sizeof(large_sizes) / sizeof(large_sizes[0]);

    double start = now_sec();
    for (int i = 0; i < 100; i++) {
        size_t size = large_sizes[i % num_large_sizes];
        void *ptr = my_alloc(size);
        if (!ptr) {
            fprintf(stderr, "[Custom Allocator Large] Allocation failed at iteration %d (size=%zu)\n", i, size);
            break;
        }
        memset(ptr, 0xDD, size);
        my_free(ptr);
    }
    double end = now_sec();
    printf("[Custom Allocator Large] Time: %.6f seconds\n", end - start);

    start = now_sec();
    for (int i = 0; i < 100; i++) {
        size_t size = large_sizes[i % num_large_sizes];
        void *ptr = malloc(size);
        if (!ptr) {
            fprintf(stderr, "[Standard malloc Large] Allocation failed at iteration %d (size=%zu)\n", i, size);
            break;
        }
        memset(ptr, 0xDD, size);
        free(ptr);
    }
    end = now_sec();
    printf("[Standard malloc Large] Time: %.6f seconds\n", end - start);
}
int main() {
    printf("=== Performance Comparison ===\n");
    benchmark_malloc();
    benchmark_custom();
    printf("\n=== Multi-threaded Benchmarks ===\n");
    benchmark_malloc_multithreaded();
    benchmark_custom_multithreaded();

    stress_test();
    thread_cache_cleanup();
    benchmark_large_allocs();

    printf("\n=== Final Allocator Status ===\n");
    print_allocator_status();

    allocator_cleanup();
    return 0;
}
