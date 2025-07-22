/**
 * @file kpool.h
 * @brief Kernel Memory Pool Interface
 *
 * Declares structures and functions for memory pool allocation and management in the kernel.
 */
#ifndef KPOOL_H
#define KPOOL_H

#include <kint.h>
#include <kmath.h>
#include <memory/kglobals.h>
#include <memory/paging.h>

typedef struct kernel_memory_pool_t
{
    void* pool_base;
    void* alloc_ptr;
    void* free_stack_top;
    void* free_stack_limit;
    size_t obj_size;
} kernel_memory_pool_t;

kernel_memory_pool_t* pool_create(uint64_t element_size, uint64_t alignment);

void* pool_allocate(kernel_memory_pool_t* pool);

void pool_free(void* ptr);

#endif
