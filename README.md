# Thread-Safe Custom Memory Allocator

A custom, thread-safe memory allocator in C, featuring slab allocation, per-thread caches, and robust multi-threaded support. This project is designed for high performance and low fragmentation, and is benchmarked against the standard `malloc`/`free` routines.

## Features

- **Thread Safety:** Per-thread caches and mutex-protected global structures ensure safe concurrent memory allocation and deallocation.
- **Slab Allocation:** Efficiently groups small allocations into slabs to minimize fragmentation and system calls.
- **Custom Metadata Management:** Avoids standard library allocation for metadata, reducing dependencies and recursion risk.
- **Performance Optimizations:** Outperforms or matches standard allocators for small and multi-threaded workloads.
- **Comprehensive Testing:** Includes single-threaded, multi-threaded, stress, and large allocation benchmarks.
- **Robust Error Handling:** All system calls are checked with clear error messages.
- **Consistent Code Style:** Readable, maintainable, and well-documented code.

## Build & Run

### Requirements

- GCC or compatible C compiler
- POSIX-compliant system (Linux recommended)
- `make`

### Commands

- **Compile:**  
  `make`
- **Run tests/benchmarks:**  
  `make run`
- **Clean build artifacts:**  
  `make clean`

## Benchmark Results

The allocator is benchmarked against standard `malloc`/`free` in various scenarios, including single-threaded, multi-threaded, stress, and large allocation patterns. See the output of `make run` for detailed results.

## Design Overview

- **Slab Allocator:** Groups small allocations into fixed-size slabs for efficiency.
- **Thread-Local Caches:** Reduces lock contention, improving multi-threaded performance.
- **Global Free Lists:** Mutex-protected per-size-class lists.
- **Custom Metadata Allocator:** Manages allocator metadata without using standard `malloc`.

## Project Structure

| File         | Description                                      |
|--------------|--------------------------------------------------|
| `alloc.h`    | Allocator API and structures                     |
| `alloc.c`    | Core allocator implementation                    |
| `test.c`     | Benchmark and test suite                         |
| `README.md`  | Project documentation                            |

## Limitations & Future Work

- No support for `realloc` or aligned allocations.
- No memory compaction or advanced fragmentation mitigation.
- Large allocations are not pooled.
- Not tested on non-Linux platforms.

## License

MIT License

## Author

PRIYANSHU MORBAITA
priyanshumorbaita@gmail.com
