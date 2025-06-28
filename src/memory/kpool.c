#include <memory/kpool.h>
#include <misc/debug.h>

#define POOL_SIZE_BYTES 0x10000000000 /* 1tb */

kernel_memory_pool_t* pool_create(uint64_t element_size, uint64_t alignment)
{
    kernel_memory_pool_t* pool = (kernel_memory_pool_t*)(0xFFFF900000000000 + 0x10000000000 * (*MEMORY_POOL_COUNTER)++);

    void* page = pages_allocatePage(PAGE_SIZE_4KB);
    pageTable_addPage(KERNEL_PAGE_TABLE, pool, (uint64_t)page / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB, 0);

    pool->pool_base = pool;
    pool->alloc_ptr = ALIGN_UP((uint64_t)pool + sizeof(kernel_memory_pool_t), alignment); // Start after metadata
    pool->free_stack_top = (char*)pool + POOL_SIZE_BYTES;                                 // Top of 1TB space
    pool->obj_size = ALIGN_UP(element_size, alignment);
    pool->free_stack_limit = pool->free_stack_top;

    return pool;
}

void* pool_allocate(kernel_memory_pool_t* pool)
{
    // First try the free stack
    if (pool->free_stack_top < (char*)pool->pool_base + POOL_SIZE_BYTES)
    {
        void* ptr = *(void**)pool->free_stack_top;
        pool->free_stack_top = (char*)pool->free_stack_top + sizeof(void*);
        return ptr;
    }

    void* aligned_addr = pool->alloc_ptr;

    // Check if we need a new page
    if ((char*)aligned_addr + pool->obj_size > (char*)pool->pool_base + POOL_SIZE_BYTES)
    {
        return NULL; // Pool exhausted
    }

    // Check if we're crossing a page boundary
    if (((uintptr_t)aligned_addr / PAGE_SIZE_4KB) != (((uintptr_t)aligned_addr + pool->obj_size - 1) / PAGE_SIZE_4KB))
    {
        // Allocate new page if needed
        void* new_page = pages_allocatePage(PAGE_SIZE_4KB);
        pageTable_addPage(KERNEL_PAGE_TABLE, ALIGN_UP((uint64_t)pool->alloc_ptr, PAGE_SIZE_4KB), (uint64_t)new_page / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB, 0);
    }

    void* ptr = (void*)aligned_addr;
    pool->alloc_ptr = (char*)aligned_addr + pool->obj_size;

    return ptr;
}

void pool_free(void* ptr)
{
    kernel_memory_pool_t* pool = ALIGN_DOWN((uint64_t)ptr, 0x10000000000 /* 1tb */);
    // add a new page if needed
    if (pool->free_stack_limit == pool->free_stack_top)
    {
        void* free_page = pages_allocatePage(PAGE_SIZE_4KB);
        pool->free_stack_limit -= PAGE_SIZE_4KB;
        pageTable_addPage(KERNEL_PAGE_TABLE, pool->free_stack_limit, (uint64_t)free_page / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB, 0);
    }

    // Push onto free stack
    pool->free_stack_top = (char*)pool->free_stack_top - sizeof(void*);
    *(void**)pool->free_stack_top = ptr;
}