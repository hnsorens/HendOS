/**
 * @file paging.c
 * @brief Kernel Physical Memory Management Implementation
 * 
 * Implements physical page allocation, deallocation, and management
 * with support for both 4kb and 2mb page sizes.
 */

#include <memory/paging.h>
#include <memory/kmemory.h>
#include <boot/bootServices.h>
#include <memory/memoryMap.h>
#include <memory/kglobals.h>

/* Page size constants */
#define PAGE_SIZE_4KB 0x1000
#define PAGE_SIZE_2MB 0x200000
#define PAGES_PER_2MB (PAGE_SIZE_2MB / PAGE_SIZE_4KB) /* 512 pages */

/* ==================== Bitmap Operations ==================== */

/**
 * @brief Set a bit in the page bitmap
 * @param bitmap Pointer to the bitmap array
 * @param index Bit index to set
 */
static void bitmap_set(uint64_t* bitmap, uint64_t index)
{
    bitmap[index / 64] |= 1ULL << (index % 64);
}

/**
 * @brief Clear a bit in the page bitmap
 * @param bitmap Pointer to the bitmap array
 * @param index Bit index to clear
 */
static void bitmap_clear(uint64_t* bitmap, uint64_t index)
{
    bitmap[index / 64] &= ~(1ULL << (index % 64));
}

/**
 * @brief Test a bit in the page bitmap
 * @param bitmap Pointer to the bitmap array
 * @param index Bit index to test
 * @return 1 if bit is set, 0 otherwise
 */
static int bitmap_test(const uint64_t* bitmap, uint64_t index)
{
    return (bitmap[index / 64] >> (index % 64)) & 1;
}

/* ==================== Page Management ==================== */

/**
 * @brief Reserve a range of physical pages
 * @param page_start Starting page index
 * @param page_count Number of pages to reserve
 * @param page_size Size of each page (PAGE_SIZE_4KB or PAGE_SIZE_2MB)
 */
void pages_reservePage(uint64_t page_start, uint64_t page_count, uint64_t page_size)
{
    uint64_t* bitmap = 0;
    if (page_size == PAGE_SIZE_2MB)
    {
        bitmap = *BITMAP_2MB;
    }
    else if (page_size == PAGE_SIZE_4KB)
    {
        bitmap = *BITMAP_4KB;
    }
    else
    {
        return; /* Invalid page size */
    }

    /* Mark all pages in the range as reserved */
    for (int i = 0; i < page_count; i++)
    {
        bitmap_set(bitmap, page_start + i);
    }
}

/**
 * @brief Initialize the page allocation tables
 * @param memoryStart Starting address for allocation tables
 * @param totalMemory Total available physical memory in bytes
 * @param regions Array of memory regions
 * @param regions_count Number of memory regions
 */
void pages_initAllocTable(uint64_t* memoryStart, uint64_t totalMemory, MemoryRegion* regions, size_t regions_count)
{
    /* Clear the entire allocation table */
    kmemset(PAGE_ALLOCATION_TABLE_START, 0, PAGE_ALLOCATION_TABLE_SIZE);

    /* Calculate total number of pages */
    *NUM_2MB_PAGES = totalMemory / PAGE_SIZE_2MB;
    *NUM_4KB_PAGES = totalMemory / PAGE_SIZE_4KB;

    /*
     * Memory layout for allocation tables:
     * [ bitmap_2mb (num_2mb_pages bits) ]
     * [ bitmap_4kb (num_4kb_pages bits) ]
     * [ free_stack_2mb (uint32_t stack) ]
     * [ free_stack_4kb (uint32_t stack) ]
     */

    /* Calculate bitmap sizes in uint64_t units */
    uint64_t bitmap_2mb_size = (*NUM_2MB_PAGES) / 64;
    uint64_t bitmap_4kb_size = (*NUM_4KB_PAGES) / 64;

    /* Set up bitmap pointers */
    *BITMAP_2MB = (uint64_t*)PAGE_ALLOCATION_TABLE_START;
    *BITMAP_4KB = (uint64_t*)PAGE_ALLOCATION_TABLE_START + bitmap_2mb_size;

    /* Set up free stacks */
    *FREE_STACK_2MB = (uint64_t*)((uint64_t*)PAGE_ALLOCATION_TABLE_START + bitmap_2mb_size + bitmap_4kb_size);
    *FREE_STACK_4KB = (uint32_t*)((uint8_t*)(*FREE_STACK_2MB) + (*NUM_2MB_PAGES) * sizeof(uint32_t));

    /* Initialize stack tops */
    *FREE_STACK_2MB_TOP = 0;
    *FREE_STACK_4KB_TOP = 0;
}

/**
 * @brief Generate free page stacks
 * 
 * Builds stacks of free pages while maintaining consistency between
 * 4KB and 2MB page tracking.
 */
void pages_generateFreeStack()
{
    /* Build 2MB free stack - only if all contained 4KB pages are free */
    for (uint64_t i = 0; i < *NUM_2MB_PAGES; i++) {
        if (!bitmap_test(*BITMAP_2MB, i)) {
            /* Check all 512 corresponding 4KB pages */
            uint64_t start_4kb = i * 512;
            bool can_use = 1;

            for (uint64_t j = 0; j < 512; j++) {
                if (bitmap_test(*BITMAP_4KB, start_4kb + j)) {
                    can_use = 0;
                    break;
                }
            }
            if (can_use) {
                (*FREE_STACK_2MB)[(*FREE_STACK_2MB_TOP)++] = (uint32_t)i;
            } else {
                /* Mark 2MB page as used since some 4KB pages are allocated */
                bitmap_set(*BITMAP_2MB, i);
            }
        }
    }

    /* Build 4KB free stack - only if containing 2MB page is free */
    for (uint64_t i = 0; i < *NUM_4KB_PAGES; i++) {
        if (!bitmap_test(*BITMAP_4KB, i)) {
            uint64_t page_2mb = i / 512;

            if (!bitmap_test(*BITMAP_2MB, page_2mb)) 
            {
                (*FREE_STACK_4KB)[(*FREE_STACK_4KB_TOP)++] = (uint32_t)i;
            } 
            else 
            {
                /* Mark 4KB page as used since 2MB page is allocated */
                bitmap_set(*BITMAP_4KB, i);
            }
        }
    }
}

/**
 * @brief Allocate a physical page
 * @param page_size Size of page to allocate (PAGE_SIZE_4KB or PAGE_SIZE_2MB)
 * @return Physical address of allocated page, NULL if allocation failed
 */
void* pages_allocatePage(uint64_t page_size)
{
    if (page_size == PAGE_SIZE_2MB) {
        if (*FREE_STACK_2MB_TOP == 0)
        {
            return NULL;    /* No free pages */
        }

        /* Pop from 2MB free stack */
        uint64_t idx = (*FREE_STACK_2MB)[--(*FREE_STACK_2MB_TOP)];

        /* Verify all 4KB pages are free (defensive check) */
        uint64_t start_4kb = idx * 512;
        for (uint64_t i = 0; i < 512; i++) {
            if (bitmap_test(*BITMAP_4KB, start_4kb + i)) {
                return pages_allocatePage(page_size);   /* Retry */
            }
        }
        
        /* Mark 2MB page as allocated */
        bitmap_set(*BITMAP_2MB, idx);

        /* Mark all contained 4KB pages as allocated */
        for (uint64_t i = 0; i < 512; i++) {
            bitmap_set(*BITMAP_4KB, start_4kb + i);
        }
        
        return (void*)(uint64_t)(idx * (uint64_t)PAGE_SIZE_2MB);
    }
    else if (page_size == PAGE_SIZE_4KB) 
    {
        if (*FREE_STACK_4KB_TOP == 0)
        {
            return NULL;    /* No free pages */
        }

        /* Pop from 4KB free stack */
        uint64_t idx = (*FREE_STACK_4KB)[--(*FREE_STACK_4KB_TOP)];
        uint64_t page_2mb = idx / 512;

        /* Verify containing 2MB page is free */
        if (bitmap_test(*BITMAP_2MB, page_2mb)) {
            return pages_allocatePage(page_size);   /* Retry */
        }

        /* Mark 4KB page as allocated */
        bitmap_set(*BITMAP_4KB, idx);

        return (void*)(uint64_t)(idx * (uint64_t)PAGE_SIZE_4KB);
    }
    return NULL;    /* Invalid page size */
}

/**
 * @brief Free a physical page
 * @param address Physical address of page to free
 * @param page_size Size of page being freed
 */
void pages_free(void* address, uint64_t page_size)
{
    uint64_t addr = (uint64_t)address;

    if (page_size == PAGE_SIZE_2MB) 
    {
        uint64_t idx = addr / PAGE_SIZE_2MB;

        /* Check for valid allocation */
        if (!bitmap_test(*BITMAP_2MB, idx)) 
        {
            return;   /* Double free or invalid address */
        }

        /* Clear 2MB page bitmap */
        bitmap_clear(*BITMAP_2MB, idx);

        /* Clear all contained 4KB page bitmaps */
        uint64_t start_4kb = idx * 512;
        for (uint64_t i = 0; i < 512; i++) 
        {
            bitmap_clear(*BITMAP_4KB, start_4kb + i);
        }

        /* Push back onto free stack */
        (*FREE_STACK_2MB)[(*FREE_STACK_2MB_TOP)++] = (uint32_t)idx;

        /* Add 4KB pages back to free stack if not reserved */
        for (uint64_t i = 0; i < 512; i++) 
        {
            uint32_t idx_4kb = (uint32_t)(start_4kb + i);
            if (!bitmap_test(*BITMAP_4KB, idx_4kb)) 
            {
                (*FREE_STACK_4KB)[(*FREE_STACK_4KB_TOP)++] = idx_4kb;
            }
        }
    }
    else if (page_size == PAGE_SIZE_4KB) 
    {
        uint64_t idx = addr / PAGE_SIZE_4KB;

        /* Check for valid allocation */
        if (!bitmap_test(*BITMAP_4KB, idx)) 
        {
            return;   /* Double free or invalid address */
        }

        uint64_t page_2mb = idx / 512;
        if (bitmap_test(*BITMAP_2MB, page_2mb)) {
            return;   /* Containing 2MB page is allocated */
        }

        /* Clear 4KB page bitmap */
        bitmap_clear(*BITMAP_4KB, idx);

        /* Push back onto free stack */
        (*FREE_STACK_4KB)[(*FREE_STACK_4KB_TOP)++] = (uint32_t)idx;
    }
}