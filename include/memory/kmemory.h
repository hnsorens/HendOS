/**
 * @file kmemory.h
 * @brief Kernel Memory Management Interface
 *
 * Provides core memory management functions including allocation, deallocation,
 * and memory operations for the kernel.
 */

#ifndef K_MEMORY_H
#define K_MEMORY_H

#include <kint.h>

/**
 * @struct block_header_t
 * @brief Header structure for memory blocks in the heap
 *
 * This structure precedes each allocated/free memory block and contains
 * metadata used by the memory allocator.
 */
typedef struct block_header_t
{
    uint64_t size;
    struct block_header_t* next;
} __attribute__((packed)) block_header_t;

/**
 * @struct heap_data_t
 * @brief Main heap control structure
 *
 * Contains the global state of the kernel heap allocator
 */
typedef struct heap_data_t
{
    block_header_t* free_list;
} __attribute__((packed)) heap_data_t;

/* ==================== Memory Management API ==================== */

/**
 * @brief Initializes the kernel heap
 * @param start Starting address of heap memory region
 * @param size Total size of heap memory region
 */
void kinitHeap(void* start, uint64_t size);

/**
 * @brief Allocates memory from kernel heap
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* kmalloc(size_t size);

/**
 * @brief Allocate aligned memory from kernel heap
 * @param size Number of bytes to allocate
 * @param alignment Alignment required (must be power of 2)
 * @return Pointer to aligned memory or NULL on failure
 */
void* kaligned_alloc(size_t size, size_t alignment);

/**
 * @brief Free previously allocated memory
 * @param ptr Pointer to memory block to free
 */
void kfree(void* ptr);

/**
 * @brief Reallocate memory block with new size
 * @param ptr Pointer to existing memory block
 * @param size New size for memory block
 * @return Pointer to reallocated memory, or NULL on failure
 */
void* krealloc(void* ptr, size_t size);

/* ==================== Memory Operations ==================== */

/**
 * @brief Copy memory between buffers
 * @param dest Destination buffer
 * @param src Source buffer
 * @param n Number of bytes to copy
 * @return Pointer to destination buffer
 */
void* kmemcpy(void* dest, const void* src, size_t n);

/**
 * @brief Optimized memory copy using SIMD instructions
 * @param dst Destination buffer
 * @param src Source buffer
 * @param size Number of bytes to copy
 */
void kmemcpySIMD(void* dst, const void* src, size_t size);

/**
 * @brief Fill memory with constant byte
 * @param ptr Pointer to memory region
 * @param value Value to set (converted to unsigned char)
 * @param n Number of bytes to set
 * @return Pointer to memory region
 */
void* kmemset(void* ptr, int value, size_t n);

/**
 * @brief Compare two memory regions
 * @param s1 First memory region
 * @param s2 Second memory region
 * @param n Number of bytes to compare
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int kmemcmp(const void* s1, const void* s2, size_t n);

#endif /* K_MEMORY_H */