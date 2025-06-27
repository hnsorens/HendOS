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

/* ==================== Constants ==================== */

#define PAGE_TABLE_ENTRIES 512          /* 512 entries per table (9-bit index) */
#define PAGE_MASK 0x000FFFFFFFFFF000ULL /* Mask for 52-bit physical address */

/* Page table entry flags (Intel Vol. 3A 4-11) */
#define PAGE_PRESENT 0x001        /* Bit 0: Present in memory */
#define PAGE_WRITABLE 0x002       /* Bit 1: Read/Write permissions */
#define PAGE_USER 0x004           /* Bit 2: User/Supervisor level */
#define PAGE_PS 0x080             /* Bit 7: Page Size (0=4KB, 1=2MB/1GB) */
#define PAGE_NO_EXEC (1ULL << 63) /* Bit 63: Execute-disable */
#define PAGE_COW (1ULL << 52)     /* Bit 52: Copy On Write */

/* Virtual address bitfield extraction (Intel Vol. 3A 4-12) */
#define PML4_INDEX(x) (((x) >> 39) & 0x1FF) /* Bits 39-47: PML4 index */
#define PDPT_INDEX(x) (((x) >> 30) & 0x1FF) /* Bits 30-38: PDPT index */
#define PD_INDEX(x) (((x) >> 21) & 0x1FF)   /* Bits 21-29: PD index */
#define PT_INDEX(x) (((x) >> 12) & 0x1FF)   /* Bits 12-20: PT index */

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

typedef struct
{
    uint64_t entry;
    uint64_t size;
} page_lookup_result_t;

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

page_table_t* pageTable_fork(page_table_t* ref, uint64_t kernel_memory_index);

page_lookup_result_t pageTable_find_entry(page_table_t* pageTable, uint64_t cr2);

#endif