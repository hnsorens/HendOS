/**
 * @file pageTable.h
 * @brief Kernel Page Table Management Interface
 *
 * Implements x86-64 page table creation, modification, and activation.
 * Supports 4KB, 2MB, and 1GB page sizes with proper memory mapping.
 *
 * The x86-64 paging hierarchy:
 * PML4 (Page Map Level 4) → PDPT (Page Directory Pointer Table)
 * → PD (Page Directory) → PT (Page Table) → Physical Page
 */

#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include <boot/bootServices.h>
#include <kint.h>
#include <memory/kmemory.h>

/* ==================== Data Structure ==================== */

/**
 * @struct page_table_t
 * @brief Root of page table along with the size its taking in memory
 */
typedef struct
{
    uint64_t size;
    uint64_t* pml4;
} page_table_t;

/* ==================== Constants ==================== */

#define PAGE_SIZE_4KB 0x1000     /* 4kb */
#define PAGE_SIZE_2MB 0x200000   /* 2mb */
#define PAGE_SIZE_1GB 0x40000000 /* 1gb */

/**
 * @brief Allocates and initializes a new page table
 * @return Pointer to new page table, NULL on failure
 */
page_table_t* pageTable_createPageTable();

/**
 * @brief Creates initial pre-kernel identity-mapped page table
 * @param start Physical address for PML4 table
 * @param total_memory Total system memory in bytes
 * @return Initialized page table
 *
 * Creates 1:1 virtual-to-physical mapping required for early boot.
 * All pages are marked as supervisor-only and read/write.
 */
page_table_t pageTable_createKernelPageTable(void* start, uint64_t total_memory);

/**
 * @brief Maps physical pages into virtual address space
 * @param pageTable Target page table structure
 * @param virtual_address Starting virtual address
 * @param page_number Starting physical page number
 * @param page_count Number of pages to map
 * @param pageSize Page size (4KB, 2MB, or 1GB)
 * @param flags Page attribute flags
 * @return 0 on success, -1 on failure
 *
 * Walks the 4-level paging structure, allocating tables as needed.
 * For 2MB/1GB pages, uses the PS bit to create large pages.
 */
int pageTable_addPage(page_table_t* pageTable,
                      void* virtual_address,
                      uint64_t page_number,
                      uint64_t page_count,
                      uint64_t page_size,
                      uint16_t flags);

/**
 * @brief Maps kernel memory regions into a page table
 * @param pageTable Target page table to modify
 *
 * Maps:
 * - EFI memory map regions
 * - Kernel heap
 * - Kernel stack
 * - Page allocation tables
 * - Framebuffer memory
 */
void pageTable_addKernel(page_table_t* pageTable);

/**
 * @brief Maps physical pages into virtual address space (used for kernel page table)
 * @param pageTable Kernal page table
 * @param virtual_address Starting virtual address
 * @param page_number Starting physical page number
 * @param page_count Number of pages to map
 * @param pageSize Page size (4KB, 2MB, or 1GB)
 * @return 0 on success, -1 on failure
 *
 * Walks the 4-level paging structure, allocating tables as needed.
 * For 2MB/1GB pages, uses the PS bit to create large pages.
 */
int pageTable_addKernelPage(page_table_t* pageTable,
                            void* virtual_address,
                            uint64_t page_number,
                            uint64_t page_count,
                            uint64_t pageSize);

/**
 * @brief Activates a page table by loading CR3
 * @param pml4 Physical address of PML4 table
 * @return 0 on success, -1 on failure
 *
 * Performs:
 * 1. CR3 load (with PCID=0)
 * 2. TLB flush via invlpg
 */
int pageTable_set(void* pageTable);

page_table_t* pageTable_fork(page_table_t* ref);

#endif