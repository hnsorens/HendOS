/**
 * @file kmemory.c
 * @brief Kernel Memory Management Implementation
 *
 * Provides core memory management functions including allocation, deallocation,
 * and memory operations for the kernel
 */

#include <kmath.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <memory/paging.h>
#include <misc/debug.h>

static int kheap_expand(size_t pages);

/* ==================== Heap Management ==================== */

void kinitHeap(void* start, uint32_t pages)
{
    // Verify 2MB alignment
    if ((uint64_t)start % PAGE_SIZE_2MB != 0)
        return;

    HEAP_DATA->heap_start = start;
    HEAP_DATA->heap_top = start;
    HEAP_DATA->free_list = NULL;

    // Initial heap expansion
    for (uint32_t i = 0; i < pages; i++)
    {
        kheap_expand(1); // Expand by 1 page (2MB)
    }
}

static int kheap_expand(size_t pages)
{
    void* virt_addr = HEAP_DATA->heap_end;
    void* first_phys_page = NULL;
    void* current_virt = virt_addr;

    // Allocate and map each page individually
    for (size_t i = 0; i < pages; i++)
    {
        void* phys_page = pages_allocatePage(PAGE_SIZE_2MB);
        if (!phys_page)
        {
            // out of memory error
            return -1;
        }

        // Track first physical page for cleanup
        if (i == 0)
        {
            first_phys_page = phys_page;
        }

        // Map the page
        if (pageTable_addPage(KERNEL_PAGE_TABLE, current_virt, (uint64_t)phys_page / PAGE_SIZE_2MB, 1, PAGE_SIZE_2MB, 0) != 0)
        {
            pages_free(phys_page, PAGE_SIZE_2MB);
            // out of memory
            return -1;
        }

        current_virt = (void*)((uint64_t)current_virt + PAGE_SIZE_2MB);
    }

    HEAP_DATA->heap_end = current_virt;
    return 0;
}

static block_header_t* find_free_block(size_t size)
{
    block_header_t* curr = HEAP_DATA->free_list;
    while (curr)
    {
        if (curr->free && curr->size >= size)
        {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

static void split_block(block_header_t* block, size_t size)
{
    size_t remaining = block->size - size - sizeof(block_header_t);
    if (remaining >= MIN_BLOCK_SIZE)
    {
        block_header_t* new_block = (block_header_t*)((uint8_t*)block + sizeof(block_header_t) + size);
        new_block->size = remaining;
        new_block->free = 1;
        new_block->next = block->next;
        block->next = new_block;
        block->size = size;
    }
    block->free = 0;
}

void* kmalloc(size_t size)
{
    if (size == 0)
        return NULL;

    size = ALIGN8(size);

    // 1. Try to reuse a free block
    block_header_t* block = find_free_block(size);
    if (block)
    {
        split_block(block, size);
        return (void*)(block + 1);
    }

    // 2. Try to allocate at heap top
    size_t required = size + sizeof(block_header_t);
    uint8_t* alloc_pos = (uint8_t*)HEAP_DATA->heap_top;
    uint8_t* new_top = alloc_pos + required;

    if (new_top <= (uint8_t*)HEAP_DATA->heap_end)
    {
        block_header_t* new_block = (block_header_t*)alloc_pos;
        new_block->size = size;
        new_block->free = 0;
        new_block->next = NULL;
        HEAP_DATA->heap_top = new_top;
        return (void*)(new_block + 1);
    }

    // 3. Need to expand heap
    size_t heap_size = (uint8_t*)HEAP_DATA->heap_end - (uint8_t*)HEAP_DATA->heap_start;
    size_t expand_pages = MAX((required + PAGE_SIZE_2MB - 1) / PAGE_SIZE_2MB,
                              (heap_size / PAGE_SIZE_2MB) / 2 // Expand by 50% of current size
    );

    if (kheap_expand(expand_pages) == 0)
    {
        return kmalloc(size); // Retry after expansion
    }

    return NULL; // Out of memory
}

void* kaligned_alloc(size_t size, size_t alignment)
{
    if (alignment == 0 || (alignment & (alignment - 1)) != 0)
        return NULL;

    if (alignment < sizeof(void*))
        alignment = sizeof(void*);

    size = (size + 7) & ~7;
    size_t total_size = size + alignment + sizeof(block_header_t);

    void* ptr = kmalloc(total_size);
    if (!ptr)
        return NULL;

    uint64_t aligned_addr = ((uint64_t)ptr + sizeof(block_header_t) + alignment - 1) & ~(alignment - 1);
    block_header_t* header = (block_header_t*)(aligned_addr - sizeof(block_header_t));

    // Store original pointer for freeing
    *((void**)((uint64_t)header - sizeof(void*))) = ptr;

    return (void*)aligned_addr;
}

void kfree(void* ptr)
{
    if (!ptr)
        return;

    block_header_t* block = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    block->free = 1;

    // Add to free list (could add coalescing here)
    block->next = HEAP_DATA->free_list;
    HEAP_DATA->free_list = block;
}

void* krealloc(void* ptr, size_t size)
{
    if (!ptr)
        return kmalloc(size);
    if (size == 0)
    {
        kfree(ptr);
        return NULL;
    }

    block_header_t* header = (block_header_t*)((uint64_t)ptr - sizeof(block_header_t));
    size_t old_size = header->size;
    size = (size + 7) & ~7;

    if (size <= old_size)
        return ptr;

    void* new_ptr = kmalloc(size);
    if (!new_ptr)
        return NULL;

    kmemcpy(new_ptr, ptr, old_size);
    kfree(ptr);
    return new_ptr;
}

/**
 * @brief Copy memory between buffers
 * @param dest Destination buffer
 * @param src Source buffer
 * @param n Number of bytes to copy
 * @return Pointer to destination buffer
 */
void* kmemcpy(void* dest, const void* src, size_t n)
{
    unsigned char* d = dest;
    const unsigned char* s = src;
    for (size_t i = 0; i < n; ++i)
    {
        d[i] = s[i];
    }
    return dest;
}

/**
 * @brief Optimized memory copy using SIMD instructions
 * @param dst Destination buffer
 * @param src Source buffer
 * @param size Number of bytes to copy
 */
void kmemcpySIMD(void* dst, const void* src, size_t size)
{
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;

    size_t i = 0;

    /* Copy 16 bytes at a time using SSE */
    asm volatile("1:\n\t"
                 "cmp %[i], %[size]\n\t"
                 "jge 2f\n\t"

                 "movdqu (%[src], %[i]), %%xmm0\n\t"
                 "movdqu %%xmm0, (%[dst], %[i])\n\t"

                 "add $16, %[i]\n\t"
                 "jmp 1b\n\t"

                 "2:\n\t"
                 : [i] "+r"(i)
                 : [src] "r"(s), [dst] "r"(d), [size] "r"(size & ~15UL)
                 : "xmm0", "memory");

    // Copy the remaining bytes
    for (; i < size; ++i)
        d[i] = s[i];
}

/**
 * @brief Fill memory with constant byte
 * @param ptr Pointer to memory region
 * @param value Value to set (converted to unsigned char)
 * @param n Number of bytes to set
 * @return Pointer to memory region
 */
void* kmemset(void* ptr, int value, size_t n)
{
    unsigned char* p = ptr;
    for (size_t i = 0; i < n; ++i)
    {
        p[i] = (unsigned char)value;
    }
    return ptr;
}

/**
 * @brief Compare two memory regions
 * @param s1 First memory region
 * @param s2 Second memory region
 * @param n Number of bytes to compare
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int kmemcmp(const void* s1, const void* s2, size_t n)
{
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;

    for (size_t i = 0; i < n; ++i)
    {
        if (p1[i] != p2[i])
        {
            return (int)p1[i] - (int)p2[i];
        }
    }

    return 0;
}