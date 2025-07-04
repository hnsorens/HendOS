/**
 * @file kpool.c
 * @brief Kernel Memory Pool Implementation
 *
 * Implements a fixed-size object allocator with:
 * - Demand-paged backing storage
 * - Free object tracking
 * - 1TB virtual address space per pool
 */

#include <memory/kpool.h>
#include <misc/debug.h>

#define POOL_SIZE_BYTES 0x10000000000 /* 1TB virtual address space per pool */

/**
 * @brief Create a new memory pool
 * @param element_size Size of each object in bytes
 * @param alignment Required memory alignment
 * @return Initialized memory pool structure
 *
 * Initializes a pool with:
 * - 1TB virtual address range
 * - First physical page allocated
 * - Proper alignment of objects
 */
kernel_memory_pool_t* pool_create(uint64_t element_size, uint64_t alignment)
{
    /* Allocate virtual address space from kernel pool region */
    kernel_memory_pool_t* pool = (kernel_memory_pool_t*)(0xFFFF900000000000 + 0x10000000000 * (*MEMORY_POOL_COUNTER)++);

    /* Map first physical page for pool metadata */
    void* page = pages_allocatePage(PAGE_SIZE_4KB);
    pageTable_addPage(KERNEL_PAGE_TABLE, pool, (uint64_t)page / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB, 0);

    /* Initialize pool structure */
    pool->pool_base = pool;
    pool->alloc_ptr = ALIGN_UP((uint64_t)pool + sizeof(kernel_memory_pool_t), alignment); // Start after metadata
    pool->free_stack_top = (char*)pool + POOL_SIZE_BYTES;                                 // Top of 1TB space
    pool->obj_size = ALIGN_UP(element_size, alignment);
    pool->free_stack_limit = pool->free_stack_top;

    return pool;
}

/**
 * @brief Allocate an object from the pool
 * @param pool Pool to allocate from
 * @return Pointer to allocated object or NULL
 *
 * Allocation strategy:
 * 1. First check free list for available objects
 * 2. If none, allocate sequentially from pool
 * 3. Automatically map new physical pages when needed
 */
void* pool_allocate(kernel_memory_pool_t* pool)
{
    /* First try the free stack */
    if (pool->free_stack_top < (char*)pool->pool_base + POOL_SIZE_BYTES)
    {
        void* ptr = *(void**)pool->free_stack_top;
        pool->free_stack_top = (char*)pool->free_stack_top + sizeof(void*);
        return ptr;
    }

    void* aligned_addr = pool->alloc_ptr;

    /* Check if pool is exhausted */
    if ((char*)aligned_addr + pool->obj_size > (char*)pool->pool_base + POOL_SIZE_BYTES)
    {
        return NULL; // Pool exhausted
    }

    /* Check for page boundary crossing */
    if (((uintptr_t)aligned_addr / PAGE_SIZE_4KB) != (((uintptr_t)aligned_addr + pool->obj_size - 1) / PAGE_SIZE_4KB))
    {
        /* Allocate and map new physical page if crossing boundary */
        void* new_page = pages_allocatePage(PAGE_SIZE_4KB);
        pageTable_addPage(KERNEL_PAGE_TABLE, ALIGN_UP((uint64_t)pool->alloc_ptr, PAGE_SIZE_4KB), (uint64_t)new_page / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB, 0);
    }

    /* Allocate from sequential space */
    void* ptr = (void*)aligned_addr;
    pool->alloc_ptr = (char*)aligned_addr + pool->obj_size;

    return ptr;
}

/**
 * @brief Free an object back to the pool
 * @param ptr Pointer to object being freed
 *
 * Freeing strategy:
 * 1. Determines owning pool from object address
 * 2. Adds object to free stack (LIFO)
 * 3. Allocates new pages for free stack if needed
 */
void pool_free(void* ptr)
{
    /* Find owning pool by aligning down to 1TB boundary */
    kernel_memory_pool_t* pool = ALIGN_DOWN((uint64_t)ptr, 0x10000000000 /* 1TB */);

    /* Allocate new page for free stack if needed */
    if (pool->free_stack_limit == pool->free_stack_top)
    {
        void* free_page = pages_allocatePage(PAGE_SIZE_4KB);
        pool->free_stack_limit -= PAGE_SIZE_4KB;
        pageTable_addPage(KERNEL_PAGE_TABLE, pool->free_stack_limit, (uint64_t)free_page / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB, 0);
    }

    /* Push object onto free stack */
    pool->free_stack_top = (char*)pool->free_stack_top - sizeof(void*);
    *(void**)pool->free_stack_top = ptr;
}