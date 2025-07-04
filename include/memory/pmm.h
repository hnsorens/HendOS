/**
 * @file pmm.h
 * @brief Kernel Physical Memory Management Interface
 *
 * Procides functions for managing physical memory pages, including allocations,
 * deallocations, and reservation of memory pages of different sizes
 */

#ifndef PMM_H
#define PMM_H

#include <kint.h>
#include <memory/vmm.h>

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
void pmm_init(uint64_t* allocateTableMemoryStart, uint64_t totalMemory, MemoryRegion* regions, size_t regions_count);

/**
 * @brief Allocate a physical memory page
 * @param page_size Size of page to allocate (must be 4096 or 2097152)
 * @return Pointer to allocated page, NULL if allocation failed
 *
 * Note: The returned pointer is a physical address, not a virtual one.
 */
void* pmm_allocate(uint64_t page_size);

/**
 * @brief Free a previously allocated physical page
 * @param address Physical address of page to free
 * @param page_size Size of the page being freed
 *
 * The address and size must exactly match a previous allocation.
 */
void pmm_free(void* address, uint64_t page_size);

/**
 * @brief Reserve a range of physical pages
 * @param page_start Starting physical address of page range
 * @param page_count Number of pages to reserve
 * @param page_size Size of each page in bytes
 *
 * Marks pages as used without allocating them. Useful for hardware-reserved memory.
 */
void pmm_reserve(uint64_t page_start, uint64_t page_count, uint64_t page_size);

/**
 * @brief Generate a stack-based free page list
 *
 * Initializes or rebuilds the stack-based free page tracking mechanism.
 * This may improve allocation performance for certain workloads.
 */
void pmm_free_stack_init();

#endif /* PMM_H */