#include <stdlib.h>

typedef struct BlockHeader
{
    size_t size;
    struct BlockHeader* next;
    int free;
} BlockHeader;

#define ALIGN4(x) (((x) + 3) & ~3)
#define BLOCK_SIZE sizeof(BlockHeader)
#define PAGE_SIZE 4096

static char* heap_start = (char*)0x40000000;
static char* heap_end = (char*)0x40000000;
static BlockHeader* heap_head = NULL;

void exit(int __status)
{
    __asm__ volatile("mov $1, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)__status)
                     : "rax", "rdi");
}

int atoi(const char* __nptr)
{
    int result = 0;
    int sign = 1;
    if (*__nptr == '-')
    {
        sign = -1;
        __nptr++;
    }
    while (*__nptr >= '0' && *__nptr <= '9')
    {
        result = result * 10 + (*__nptr - '0');
        __nptr++;
    }
    return result * sign;
}

static void heap_extend(unsigned long page_count)
{
    // TODO: Extend this after the syscall has been extended
    __asm__ volatile("mov $7, %%rax\n\t"
                     "mov $0, %%rdi\n\t"
                     "mov %0, %%rsi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"(page_count * 4096)
                     : "rax", "rdi", "rsi");
    heap_end += page_count * PAGE_SIZE;
}

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

static char* get_heap_free_start()
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
    return (char*)curr + BLOCK_SIZE + curr->size;
}

static void split_block(BlockHeader* block, size_t size)
{
    if (block->size >= size + BLOCK_SIZE + 4)
    {
        BlockHeader* new_block = (BlockHeader*)((char*)block + BLOCK_SIZE + size);
        new_block->size = block->size - size - BLOCK_SIZE;
        new_block->next = block->next;
        new_block->free = 1;

        block->size = size;
        block->next = new_block;
    }
}

void* malloc(size_t __size)
{
    if (__size == 0)
        return NULL;

    __size = ALIGN4(__size);
    BlockHeader* block = find_free_block(__size);
    if (block)
    {
        // Reuse free block
        split_block(block, __size);
        block->free = 0;
        return (void*)(block + 1);
    }

    // No free block found, try to allocate at heap end
    char* free_start = get_heap_free_start();

    // Calculate total needed size including header
    size_t total_size = __size + BLOCK_SIZE;

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
    block->size = __size;
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

void* realloc(void* __ptr, size_t __size)
{
    if (!__ptr)
    {
        return malloc(__size);
    }
    if (__size == 0)
    {
        free(__ptr);
        return NULL;
    }

    BlockHeader* block = (BlockHeader*)__ptr - 1;
    if (block->size >= __size)
    {
        // Current block is already big enough
        return __ptr;
    }

    // Allocate new block
    void* __new_ptr = malloc(__size);
    if (!__new_ptr)
        return NULL;

    // Copy old data
    char* src = (char*)__ptr;
    char* dst = (char*)__new_ptr;
    size_t copy_size = block->size < __size ? block->size : __size;
    for (size_t i = 0; i < copy_size; i++)
    {
        dst[i] = src[i];
    }

    free(__ptr);
    return __new_ptr;
}

void* calloc(size_t __nmemb, size_t __size)
{
    size_t total = __nmemb * __size;
    void* __ptr = malloc(total);
    if (__ptr)
    {
        // zero initialize
        char* p = (char*)__ptr;
        for (size_t i = 0; i < total; i++)
        {
            p[i] = 0;
        }
    }
    return __ptr;
}

void free(void* __ptr)
{
    if (!__ptr)
        return;

    BlockHeader* block = (BlockHeader*)__ptr - 1;
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