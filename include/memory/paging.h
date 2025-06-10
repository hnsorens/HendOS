/**
 * @file paging.h
 * @brief Kernel Physical Memory Management Interface
 *
 * Procides functions for managing physical memory pages, including allocations,
 * deallocations, and reservation of memory pages of different sizes
 */

#ifndef K_PAGING_H
#define K_PAGING_H

#include <kint.h>
#include <memory/pageTable.h>

/**
 * @brief Initialize the physical page allocator
 * @param allocateTableMemoryStart Start address for allocation table
 * @param totalMemory Total available physical memory in bytes
 * @param regions Array of memory regions to consider
 * @param regions_count Number of memory regions
 *
 * This function initializes the physical memory allocator by setting up
 * the necessary data structures to track free and used physical pages.
 */
void pages_initAllocTable(uint64_t* allocateTableMemoryStart,
                          uint64_t totalMemory,
                          MemoryRegion* regions,
                          size_t regions_count);

/**
 * @brief Allocate a physical memory page
 * @param page_size Size of page to allocate (must be 4096 or 2097152)
 * @return Pointer to allocated page, NULL if allocation failed
 *
 * Note: The returned pointer is a physical address, not a virtual one.
 */
void* pages_allocatePage(uint64_t page_size);

/**
 * @brief Free a previously allocated physical page
 * @param address Physical address of page to free
 * @param page_size Size of the page being freed
 *
 * The address and size must exactly match a previous allocation.
 */
void pages_free(void* address, uint64_t page_size);

/**
 * @brief Reserve a range of physical pages
 * @param page_start Starting physical address of page range
 * @param page_count Number of pages to reserve
 * @param page_size Size of each page in bytes
 *
 * Marks pages as used without allocating them. Useful for hardware-reserved memory.
 */
void pages_reservePage(uint64_t page_start, uint64_t page_count, uint64_t page_size);

/**
 * @brief Generate a stack-based free page list
 *
 * Initializes or rebuilds the stack-based free page tracking mechanism.
 * This may improve allocation performance for certain workloads.
 */
void pages_generateFreeStack();

#endif /* K_PAGING_H */