/**
 * @file pageTable.c
 * @brief Kernel Page Table Management Implementation
 *
 * Implements x86-64 page table creation, modification, and activation.
 * Supports 4KB, 2MB, and 1GB page sizes with proper memory mapping.
 *
 * The x86-64 paging hierarchy:
 * PML4 (Page Map Level 4) → PDPT (Page Directory Pointer Table)
 * → PD (Page Directory) → PT (Page Table) → Physical Page
 */

#include <boot/bootServices.h>
#include <memory/kglobals.h>
#include <memory/memoryMap.h>
#include <memory/pageTable.h>
#include <memory/paging.h>

/* ==================== Constants ==================== */

#define PAGE_TABLE_ENTRIES 512          /* 512 entries per table (9-bit index) */
#define PAGE_MASK 0x000FFFFFFFFFF000ULL /* Mask for 52-bit physical address */

/* Page table entry flags (Intel Vol. 3A 4-11) */
#define PAGE_PRESENT 0x001        /* Bit 0: Present in memory */
#define PAGE_WRITABLE 0x002       /* Bit 1: Read/Write permissions */
#define PAGE_USER 0x004           /* Bit 2: User/Supervisor level */
#define PAGE_PS 0x080             /* Bit 7: Page Size (0=4KB, 1=2MB/1GB) */
#define PAGE_NO_EXEC (1ULL << 63) /* Bit 63: Execute-disable */

/* Virtual address bitfield extraction (Intel Vol. 3A 4-12) */
#define PML4_INDEX(x) (((x) >> 39) & 0x1FF) /* Bits 39-47: PML4 index */
#define PDPT_INDEX(x) (((x) >> 30) & 0x1FF) /* Bits 30-38: PDPT index */
#define PD_INDEX(x) (((x) >> 21) & 0x1FF)   /* Bits 21-29: PD index */
#define PT_INDEX(x) (((x) >> 12) & 0x1FF)   /* Bits 12-20: PT index */

/* =============== Internal Data Structures =================== */

/**
 * @struct page_table_indices_t
 * @brief Decomposed virtual address components for x86-64 paging
 */
typedef struct page_table_indices_t
{
    uint16_t pml4_index; ///< PML4 (Page Map Level 4) index
    uint16_t pdpt_index; ///< PDPT (Page Directory Pointer Table) index
    uint16_t pd_index;   ///< PD (Page Directory) index
    uint16_t pt_index;   ///< PT (Page Table) index
    uint16_t offset;     ///< Page offset
} page_table_indices_t;

/* ==================== Internal Functions ==================== */

/**
 * @brief Extracts indices from virtual address for page table traversal
 * @param virtual_address 48-bit canonical virtual address
 * @return Decomposed indices structure
 *
 * x86-64 uses 4-level paging:
 * [47:39] PML4 index → [38:30] PDPT index →
 * [29:21] PD index → [20:12] PT index → [11:0] Offset
 */
page_table_indices_t extract_indices(uint64_t virtual_address)
{
    page_table_indices_t indices;
    indices.offset = virtual_address & 0xFFF;         /* Page offset (bits 0-11) */
    indices.pt_index = PT_INDEX(virtual_address);     /* Page Table index */
    indices.pd_index = PD_INDEX(virtual_address);     /* Page Directory index */
    indices.pdpt_index = PDPT_INDEX(virtual_address); /* PDPT index */
    indices.pml4_index = PML4_INDEX(virtual_address); /* PML4 index */
    return indices;
}

/* ==================== Public API Implementation ==================== */

/**
 * @brief Allocates and initializes a new page table
 * @return Pointer to new page table, NULL on failure
 */
page_table_t* pageTable_createPageTable()
{
    /* Allocate page table control structure */
    page_table_t* table = kmalloc(sizeof(page_table_t));
    table->size = 0; /* No pages allocated yet */
    table->pml4 = 0; /* PML4 not yet created */
    return table;
}

/**
 * @brief Creates initial pre-kernel identity-mapped page table
 * @param start Physical address for PML4 table
 * @param total_memory Total system memory in bytes
 * @return Initialized page table
 *
 * Creates 1:1 virtual-to-physical mapping required for early boot.
 * All pages are marked as supervisor-only and read/write.
 */
page_table_t pageTable_createKernelPageTable(void* start, uint64_t total_memory)
{
    page_table_t table;
    table.size = PAGE_SIZE_4KB; /* First 4KB used for PML4 */
    table.pml4 = start;         /* PML4 at start address */

    /* Zero out the PML4 table */
    kmemset(start, 0, PAGE_SIZE_4KB);

    /* Identity map all physical memory */
    pageTable_addKernelPage(&table, 0,                    /* Virtual = Physical */
                            0,                            /* Start at physical 0 */
                            total_memory / PAGE_SIZE_4KB, /* Number of 4KB pages */
                            PAGE_SIZE_4KB);               /* 4KB granularity */
    return table;
}

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
                      uint64_t pageSize,
                      uint16_t flags)
{
    /* Parameter validation */
    if (!pageTable)
    {
        return -1;
    }

    /* Initialize PML4 if not present */
    if (!pageTable->pml4)
    {
        pageTable->pml4 = pages_allocatePage(PAGE_SIZE_4KB);
        if (!pageTable->pml4)
            return -1; /* Count not allocate page entry */

        pageTable->size = PAGE_SIZE_4KB;
        kmemset(pageTable->pml4, 0, PAGE_SIZE_4KB);
    }

    uint64_t vaddr = (uint64_t)virtual_address;
    uint64_t* pml4 = pageTable->pml4;

    /* Map each page in the range */
    for (uint64_t i = 0; i < page_count; ++i)
    {
        uint64_t curr_vaddr = vaddr + i * pageSize;
        uint64_t phys_addr = page_number * pageSize + i * pageSize;
        page_table_indices_t idx = extract_indices(curr_vaddr);

        /* --- PML4 → PDPT --- */
        uint64_t* pdpt;
        if (!(pml4[idx.pml4_index] & PAGE_PRESENT))
        {
            /* Allocate new PDPT */
            pdpt = pages_allocatePage(PAGE_SIZE_4KB);
            if (!pdpt)
                return -1;

            /* Set entry with flags */
            pml4[idx.pml4_index] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITABLE | 0x4; // flags;
            pml4[idx.pml4_index] &= ~(1ULL << 63);

            pageTable->size += PAGE_SIZE_4KB;
            kmemset(pdpt, 0, PAGE_SIZE_4KB);
        }
        else
        {
            /* Existing PDPT - update flags */
            pml4[idx.pml4_index] |= 0x4; // flags;
            pml4[idx.pml4_index] &= ~(1ULL << 63);
            pdpt = (uint64_t*)(pml4[idx.pml4_index] & PAGE_MASK);
        }

        /* Handle 1GB pages (PS bit set in PDPT entry) */
        if (pageSize == PAGE_SIZE_1GB)
        {
            pdpt[idx.pdpt_index] =
                (phys_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_PS | 0x4; // flags;
            pdpt[idx.pdpt_index] &= ~(1ULL << 63);
            continue; /* Skip lower levels */
        }

        /* --- PDPT → PD --- */
        uint64_t* pd;
        if (!(pdpt[idx.pdpt_index] & PAGE_PRESENT))
        {
            /* Allocate new Page Directory */
            pd = pages_allocatePage(PAGE_SIZE_4KB);
            if (!pd)
                return -1;

            pdpt[idx.pdpt_index] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITABLE | 0x4; // flags;
            pdpt[idx.pdpt_index] &= ~(1ULL << 63);
            pageTable->size += PAGE_SIZE_4KB;
            kmemset(pd, 0, PAGE_SIZE_4KB);
        }
        else
        {
            pdpt[idx.pdpt_index] |= 0x4; // flags;
            pdpt[idx.pdpt_index] &= ~(1ULL << 63);
            pd = (uint64_t*)(pdpt[idx.pdpt_index] & PAGE_MASK);
        }

        /* Handle 2MB pages (PS bit set in PD entry) */
        if (pageSize == PAGE_SIZE_2MB)
        {
            pd[idx.pd_index] =
                (phys_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_PS | 0x4; // flags;
            pd[idx.pd_index] &= ~(1ULL << 63);
            continue; /* Skip lower levels */
        }

        /* --- PD → PT (4KB pages) --- */
        uint64_t* pt;
        if (!(pd[idx.pd_index] & PAGE_PRESENT))
        {
            /* Allocate new Page Table */
            pt = pages_allocatePage(PAGE_SIZE_4KB);
            if (!pt)
                return -1;

            pd[idx.pd_index] = (uint64_t)pt | PAGE_PRESENT | PAGE_WRITABLE | 0x4; // flags;
            pd[idx.pd_index] &= ~(1ULL << 63);
            pageTable->size += PAGE_SIZE_4KB;
            kmemset(pt, 0, PAGE_SIZE_4KB);
        }
        else
        {
            pd[idx.pd_index] |= 0x4; // flags;
            pd[idx.pd_index] &= ~(1ULL << 63);
            pt = (uint64_t*)(pd[idx.pd_index] & PAGE_MASK);
        }

        /* Set final page table entry */
        pt[idx.pt_index] = (phys_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE | 0x4; // flags;
        pd[idx.pd_index] &= ~(1ULL << 63);
    }

    return 0;
}

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
void pageTable_addKernel(page_table_t* pageTable)
{
    if (!pageTable)
        return;

    /* Map all EFI memory regions (except conventional memory) */
    uint64_t numRegions = PREBOOT_INFO->MemoryMapSize / PREBOOT_INFO->DescriptorSize;
    EFI_MEMORY_DESCRIPTOR* entry =
        (EFI_MEMORY_DESCRIPTOR*)((char*)PREBOOT_INFO->MemoryMap + KERNEL_CODE_START);

    for (UINTN i = 0; i < numRegions; i++)
    {
        if (entry->Type != EfiConventionalMemory)
        {
            uint64_t start_4kb = entry->PhysicalStart / PAGE_SIZE_4KB;
            uint64_t count_4kb = entry->NumberOfPages;
            pageTable_addPage(pageTable, entry->PhysicalStart + KERNEL_CODE_START, start_4kb,
                              count_4kb, PAGE_SIZE_4KB, 0);
        }
        entry = (EFI_MEMORY_DESCRIPTOR*)((uint8_t*)entry + PREBOOT_INFO->DescriptorSize);
    }

    /* Map critical kernel regions */
    MemoryRegion* reg = MEMORY_REGIONS;

    /* Kernel heap */
    pageTable_addPage(pageTable, KERNEL_HEAP_START, reg[0].base / PAGE_SIZE_4KB,
                      reg[0].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB, 0);

    /* Kernel stack */
    pageTable_addPage(pageTable, KERNEL_STACK_START, reg[1].base / PAGE_SIZE_4KB,
                      reg[1].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB, 0);

    /* Page allocation table */
    pageTable_addPage(pageTable, PAGE_ALLOCATION_TABLE_START, reg[2].base / PAGE_SIZE_4KB,
                      reg[2].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB, 0);

    /* Global Variables */
    pageTable_addPage(pageTable, GLOBAL_VARS_START, reg[4].base / PAGE_SIZE_4KB,
                      reg[4].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB, 0);

    /* Framebuffer */
    pageTable_addPage(pageTable, FRAMEBUFFER_START, reg[5].base / PAGE_SIZE_4KB,
                      reg[5].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB, 0);
}

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
                            uint64_t pageSize)
{
    /* Parameter validation */
    if (!pageTable)
    {
        return -1;
    }

    uint64_t vaddr = (uint64_t)virtual_address;
    uint64_t* pml4 = pageTable->pml4;

    /* Map each page in the range */
    for (uint64_t i = 0; i < page_count; ++i)
    {
        uint64_t curr_vaddr = vaddr + i * pageSize;
        uint64_t phys_addr = page_number * pageSize + i * pageSize;
        page_table_indices_t idx = extract_indices(curr_vaddr);

        /* --- PML4 → PDPT --- */
        uint64_t* pdpt;
        if (!(pml4[idx.pml4_index] & PAGE_PRESENT))
        {
            /* Allocate new PDPT */
            pdpt = (uint64_t*)((uint64_t)pml4 + pageTable->size);
            pageTable->size += PAGE_SIZE_4KB;
            kmemset(pdpt, 0, PAGE_SIZE_4KB);

            /* Set entry with flags */
            pml4[idx.pml4_index] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITABLE;
        }
        else
        {
            pdpt = (uint64_t*)(pml4[idx.pml4_index] & PAGE_MASK);
        }

        /* Handle 1GB pages (PS bit set in PDPT entry) */
        if (pageSize == PAGE_SIZE_1GB)
        {
            pdpt[idx.pdpt_index] = (phys_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_PS;
            continue; /* Skip lower levels */
        }

        /* --- PDPT → PD --- */
        uint64_t* pd;
        if (!(pdpt[idx.pdpt_index] & PAGE_PRESENT))
        {
            /* Allocate new Page Directory */
            pd = (uint64_t*)((uint64_t)pml4 + pageTable->size);
            pageTable->size += PAGE_SIZE_4KB;
            kmemset(pd, 0, PAGE_SIZE_4KB);

            /* Set entry with flags */
            pdpt[idx.pdpt_index] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITABLE;
        }
        else
        {
            pd = (uint64_t*)(pdpt[idx.pdpt_index] & PAGE_MASK);
        }

        /* Handle 2MB pages (PS bit set in PD entry) */
        if (pageSize == PAGE_SIZE_2MB)
        {
            pd[idx.pd_index] = (phys_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_PS;
            continue; /* Skip lower levels */
        }

        /* --- PD → PT (4KB pages) --- */
        uint64_t* pt;
        if (!(pd[idx.pd_index] & PAGE_PRESENT))
        {
            /* Allocate new Page Table */
            pt = (uint64_t*)((uint64_t)pml4 + pageTable->size);
            pageTable->size += PAGE_SIZE_4KB;
            kmemset(pt, 0, PAGE_SIZE_4KB);

            /* Set entry with flags */
            pd[idx.pd_index] = (uint64_t)pt | PAGE_PRESENT | PAGE_WRITABLE;
        }
        else
        {
            pt = (uint64_t*)(pd[idx.pd_index] & PAGE_MASK);
        }

        /* Set final page table entry */
        pt[idx.pt_index] = (phys_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    return 0;
}

/**
 * @brief Activates a page table by loading CR3
 * @param pml4 Physical address of PML4 table
 * @return 0 on success, -1 on failure
 *
 * Performs:
 * 1. CR3 load (with PCID=0)
 * 2. TLB flush via invlpg
 */
int pageTable_set(void* pml4)
{
    if (!pml4)
        return -1;

    /* Load new page table root */
    __asm__ volatile("mov %0, %%cr3" : : "r"((uint64_t)pml4));
    /* Invalidate TLB entry for address 0 */
    __asm__ volatile("invlpg (0)" ::: "memory");

    return 0;
}