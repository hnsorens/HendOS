#ifndef KPOOL_H
#define KPOOL_H

#include <kint.h>
#include <kmath.h>
#include <memory/kglobals.h>
#include <memory/paging.h>

typedef struct kernel_memory_pool_t
{
    uint32_t pool_element_size;
    uint32_t element_size;
    uint32_t element_count;
    uint32_t max_elements;
    uint32_t page_count;
    uint64_t* free_stack_pointer;
} kernel_memory_pool_t;

kernel_memory_pool_t* pool_create(uint64_t element_size, uint64_t alignment, uint64_t max_elements);

void* pool_allocate(kernel_memory_pool_t* pool);

void pool_free(void* ptr);

#endif
