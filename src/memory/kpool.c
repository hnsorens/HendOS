#include <memory/kpool.h>

kernel_memory_pool_t* pool_create(uint64_t element_size, uint64_t alignment, uint64_t max_elements)
{
    kernel_memory_pool_t* pool = (kernel_memory_pool_t*)(0x10000000000 * (*MEMORY_POOL_COUNTER)++);

    void* front_page = pages_allocatePage(PAGE_SIZE_4KB);
    pageTable_addKernelPage(KERNEL_PAGE_TABLE, pool, (uint64_t)front_page / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB);

    pool->max_elements = max_elements;
    pool->element_size = element_size;
    pool->pool_element_size = ALIGN_UP(element_size, alignment);
    pool->element_count = 0;
    pool->page_count = 1;

    // Allocate pages for free stack
    uint64_t free_stack_bytes = max_elements * sizeof(uint64_t);
    uint64_t free_stack_page_count = ALIGN_UP(free_stack_bytes, PAGE_SIZE_4KB) / PAGE_SIZE_4KB;
    uint64_t pool_top = (uint64_t)pool + 0x10000000000;

    uint64_t free_stack_front = pool_top - (free_stack_page_count * PAGE_SIZE_4KB);
    for (int i = 0; i < free_stack_page_count; i++)
    {
        void* page = pages_allocatePage(PAGE_SIZE_4KB);
        pageTable_addKernelPage(KERNEL_PAGE_TABLE, pool, (free_stack_front + i * PAGE_SIZE_4KB) / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB);
    }

    // Initialize free stack (push all indices)
    for (uint64_t i = 0; i < max_elements; i++)
    {
        ((uint64_t*)pool_top)[-1 - i] = i;
    }

    pool->free_stack_pointer = (uint64_t*)(pool_top - max_elements * sizeof(uint64_t));

    return pool;
}

void* pool_allocate(kernel_memory_pool_t* pool)
{
    if (pool->element_count >= pool->max_elements)
        return NULL; // No more space

    uint64_t index = *pool->free_stack_pointer;
    pool->free_stack_pointer++;
    pool->element_count++;

    return (void*)((uint64_t)pool + index * pool->pool_element_size);
}

void pool_free(void* ptr)
{
    kernel_memory_pool_t* pool = (kernel_memory_pool_t*)ALIGN_DOWN((uint64_t)ptr, 0x10000000000);

    uint64_t index = ((uint64_t)ptr - (uint64_t)pool) / pool->pool_element_size;

    pool->free_stack_pointer--;
    *pool->free_stack_pointer = index;
    pool->element_count--;
}