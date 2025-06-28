/**
 * @file kmemory.c
 * @brief Kernel Memory Management Implementation
 *
 * Provides core memory management functions including allocation, deallocation,
 * and memory operations for the kernel
 */

#include <memory/kglobals.h>
#include <memory/kmemory.h>

/* ==================== Heap Management ==================== */

/**
 * @brief Initializes the kernel heap
 * @param start Starting address of heap memory region
 * @param size Total size of heap memory region
 */
void kinitHeap(void* start, uint64_t size)
{
    HEAP_DATA->free_list = (block_header_t*)start;
    HEAP_DATA->free_list->size = size;
    HEAP_DATA->free_list->next = 0;
}

/**
 * @brief Allocates memory from kernel heap
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* kmalloc(size_t size)
{
    /* Round size up to nearest 8-byte boundary for alignment */
    size = (size + 7) & ~7;

    block_header_t* prev = NULL;
    block_header_t* curr = HEAP_DATA->free_list;

    /* Search free list for suitable block*/
    while (curr != NULL)
    {
        if (curr->size >= size + sizeof(block_header_t))
        {
            /* Split block if there's not enough space */
            if (curr->size > size + sizeof(block_header_t))
            {
                block_header_t* next_block = (block_header_t*)((uint64_t)curr + size + sizeof(block_header_t));
                next_block->size = curr->size - size - sizeof(block_header_t);
                next_block->next = curr->next;

                curr->size = size;
                curr->next = next_block;
            }

            /* Remove the block from the free list */
            if (prev != NULL)
            {
                prev->next = curr->next;
            }
            else
            {
                HEAP_DATA->free_list = curr->next;
            }

            /* Return poitner to user data area*/
            return (void*)((uint64_t)curr + sizeof(block_header_t));
        }

        prev = curr;
        curr = curr->next;
    }

    /* no suitable block found */
    return NULL;
}

/**
 * @brief Allocate aligned memory from kernel heap
 * @param size Number of bytes to allocate
 * @param alignment Alignment required (must be power of 2)
 * @return Pointer to aligned memory or NULL on failure
 */
void* kaligned_alloc(size_t size, size_t alignment)
{
    /* validate alignment is power of 2*/
    if (alignment == 0 || (alignment & (alignment - 1)) != 0)
    {
        return NULL;
    }

    /* round size up for alignment and header */
    size = (size + 7) & ~7;
    size_t total_size = size + alignment + sizeof(void*);

    block_header_t* prev = NULL;
    block_header_t* curr = HEAP_DATA->free_list;

    while (curr != NULL)
    {
        if (curr->size >= total_size)
        {
            /* Calculate alignment address */
            uint64_t aligned_address = ((uint64_t)curr + sizeof(block_header_t) + alignment - 1) & ~(alignment - 1);
            size_t padding = aligned_address - (uint64_t)curr - sizeof(block_header_t);

            /* Check if there's enough space for the alignment and size */
            if (curr->size >= padding + size + sizeof(block_header_t))
            {
                /* split block if remainging space is sufficient */
                if (curr->size > padding + size + sizeof(block_header_t))
                {
                    block_header_t* next_block = (block_header_t*)(aligned_address + size);
                    next_block->size = curr->size - padding - size - sizeof(block_header_t);
                    next_block->next = curr->next;

                    curr->size = padding + size;
                    curr->next = next_block;
                }

                /* Remove from free list */
                if (prev != NULL)
                {
                    prev->next = curr->next;
                }
                else
                {
                    HEAP_DATA->free_list = curr->next;
                }

                return (void*)(aligned_address);
            }
        }

        prev = curr;
        curr = curr->next;
    }

    return NULL;
}

/**
 * @brief Free previously allocated memory
 * @param ptr Pointer to memory block to free
 */
void kfree(void* ptr)
{
    /* Ignore NULL pointers */
    if (ptr == NULL)
    {
        return;
    }

    /* Get block header */
    block_header_t* block_to_free = (block_header_t*)((uint64_t)ptr - sizeof(block_header_t));

    /* Add to front of free list */
    block_to_free->next = HEAP_DATA->free_list;
    HEAP_DATA->free_list = block_to_free;
}

/**
 * @brief Reallocate memory block with new size
 * @param ptr Pointer to existing memory block
 * @param size New size for memory block
 * @return Pointer to reallocated memory, or NULL on failure
 */
void* krealloc(void* ptr, size_t size)
{
    /* Handle NULL pointer case */
    if (ptr == NULL)
    {
        return kmalloc(size);
    }

    /* Handle zero size case */
    if (size == 0)
    {
        kfree(ptr);
        return NULL;
    }

    block_header_t* block_to_resize = (block_header_t*)((uint64_t)ptr - sizeof(block_header_t));
    size = (size + 7) & ~7;

    /* Return same block if already large enough */
    if (block_to_resize->size >= size)
    {
        return ptr;
    }

    /* Allocate new block and copy data */
    void* new_ptr = kmalloc(size);
    if (new_ptr != NULL)
    {
        kmemcpy(new_ptr, ptr, block_to_resize->size);
        kfree(ptr);
    }

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