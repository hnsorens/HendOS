/**
 * @file kpool.h
 * @brief Kernel Memory Pool Interface
 *
 * Provides a fixed-size object memory pool allocator for kernel use.
 * Designed for efficient allocation/deallocation of same-sized objects.
 */

#ifndef KPOOL_H
#define KPOOL_H

#include <kint.h>
#include <kmath.h>
#include <memory/kglobals.h>
#include <memory/paging.h>

/**
 * @struct kernel_memory_pool_t
 * @brief Memory pool control structure
 *
 * Manages a pool of fixed-size memory blocks with:
 * - Sequential allocation for new objects
 * - Free list for recycled objects
 * - Automatic page allocation
 */
typedef struct kernel_memory_pool_t
{
    void* pool_base;        ///< Base address of the entire pool space
    void* alloc_ptr;        ///< Pointer to next available memory for allocation
    void* free_stack_top;   ///< Top of free object stack (LIFO)
    void* free_stack_limit; ///< Limit of free stack space
    size_t obj_size;        ///< Size of each object (aligned)
} kernel_memory_pool_t;

/**
 * @brief Create a new memory pool
 * @param element_size Size of each object in the pool
 * @param alignment Alignment requirement for objects
 * @return Pointer to new memory pool or NULL on failure
 *
 * Allocates a 1TB virtual address space for the pool (backed by physical pages on demand)
 */
kernel_memory_pool_t* pool_create(uint64_t element_size, uint64_t alignment);

/**
 * @brief Allocate an object from the pool
 * @param pool Pool to allocate from
 * @return Pointer to allocated object or NULL if pool exhausted
 *
 * Tries to reuse freed objects first, then allocates new ones sequentially
 */
void* pool_allocate(kernel_memory_pool_t* pool);

/**
 * @brief Free an object back to the pool
 * @param ptr Pointer to object to free
 *
 * Returns object to free list for reuse. Automatically allocates
 * new pages if needed for free list storage.
 */
void pool_free(void* ptr);

#endif /* KPOOL_H */