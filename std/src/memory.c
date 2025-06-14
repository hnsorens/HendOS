#include <memory.h>
#include <stdint.h>

#include <stdio.h> // TODO: TAKE OUT

typedef struct BlockHeader
{
    size_t size;
    struct BlockHeader* next;
    int free;
} BlockHeader;

#define ALIGN4(x) (((x) + 3) & ~3)
#define BLOCK_SIZE sizeof(BlockHeader)
#define PAGE_SIZE 4096

uint64_t heap_start = 0x40000000;
static uint8_t* heap_end = (uint8_t*)0x40000000;
static BlockHeader* heap_head = NULL;

static void* memset(void* ptr, int value, size_t n)
{
    unsigned char* p = ptr;
    for (size_t i = 0; i < n; ++i)
    {
        p[i] = (unsigned char)value;
    }
    return ptr;
}

static void* memcpy(void* dest, const void* src, size_t n)
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
 * @brief Expands the size of heap
 */
static void heap_extend(uint64_t page_count)
{
    // TODO: Extend this after the syscall has been extended
    __asm__ volatile("mov $7, %%rax\n\t"
                     "mov $0, %%rdi\n\t"
                     "mov %0, %%rsi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((uint64_t)page_count * 4096)
                     : "rax", "rdi", "rsi");
    heap_end += page_count * PAGE_SIZE;
}

// Find a free block with at least size bytes
static BlockHeader* find_free_block(size_t size)
{
    BlockHeader* curr = heap_head;
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

// Get pointer to the end of last allocated block or heap_start if none
static uint8_t* get_heap_free_start()
{
    if (!heap_head)
    {
        return heap_start;
    }
    BlockHeader* curr = heap_head;
    while (curr->next)
    {
        curr = curr->next;
    }
    return (uint8_t*)curr + BLOCK_SIZE + curr->size;
}

// Split a free block if it's big enough to hold requested size plus a new block
static void split_block(BlockHeader* block, size_t size)
{
    if (block->size >= size + BLOCK_SIZE + 4)
    {
        BlockHeader* new_block = (BlockHeader*)((uint8_t*)block + BLOCK_SIZE + size);
        new_block->size = block->size - size - BLOCK_SIZE;
        new_block->next = block->next;
        new_block->free = 1;

        block->size = size;
        block->next = new_block;
    }
}

void* malloc(size_t size)
{
    if (size == 0)
        return NULL;

    size = ALIGN4(size);
    BlockHeader* block = find_free_block(size);
    if (block)
    {
        // Reuse free block
        split_block(block, size);
        block->free = 0;
        return (void*)(block + 1);
    }

    // No free block found, try to allocate at heap end
    uint8_t* free_start = get_heap_free_start();

    // Calculate total needed size including header
    size_t total_size = size + BLOCK_SIZE;

    // Check if we have enough space in current heap
    if (free_start + total_size > heap_end)
    {
        // Need to extend heap
        size_t needed = (free_start + total_size) - heap_end;
        size_t pages = (needed + PAGE_SIZE - 1) / PAGE_SIZE;
        heap_extend(pages);
    }

    // Allocate new block at free_start
    block = (BlockHeader*)free_start;
    block->size = size;
    block->free = 0;
    block->next = NULL;

    // Append block to list
    if (!heap_head)
    {
        heap_head = block;
    }
    else
    {
        BlockHeader* curr = heap_head;
        while (curr->next)
        {
            curr = curr->next;
        }
        curr->next = block;
    }

    return (void*)(block + 1);
}

void free(void* ptr)
{
    if (!ptr)
        return;

    BlockHeader* block = (BlockHeader*)ptr - 1;
    block->free = 1;

    // Optional: coalesce adjacent free blocks
    BlockHeader* curr = heap_head;
    while (curr && curr->next)
    {
        if (curr->free && curr->next->free)
        {
            curr->size += BLOCK_SIZE + curr->next->size;
            curr->next = curr->next->next;
        }
        else
        {
            curr = curr->next;
        }
    }
}

void* calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr)
    {
        // zero initialize
        uint8_t* p = (uint8_t*)ptr;
        for (size_t i = 0; i < total; i++)
        {
            p[i] = 0;
        }
    }
    return ptr;
}

void* realloc(void* ptr, size_t size)
{
    if (!ptr)
    {
        return malloc(size);
    }
    if (size == 0)
    {
        free(ptr);
        return NULL;
    }

    BlockHeader* block = (BlockHeader*)ptr - 1;
    if (block->size >= size)
    {
        // Current block is already big enough
        return ptr;
    }

    // Allocate new block
    void* new_ptr = malloc(size);
    if (!new_ptr)
        return NULL;

    // Copy old data
    uint8_t* src = (uint8_t*)ptr;
    uint8_t* dst = (uint8_t*)new_ptr;
    size_t copy_size = block->size < size ? block->size : size;
    for (size_t i = 0; i < copy_size; i++)
    {
        dst[i] = src[i];
    }

    free(ptr);
    return new_ptr;
}