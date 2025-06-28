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
#include <misc/debug.h>

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

/**
 * @brief Allocates and initializes a new page table
 * @return Pointer to new page table, NULL on failure
 */
page_table_t* pageTable_createPageTable()
{
    /* Allocate page table control structure */
    page_table_t* table = kmalloc(sizeof(page_table_t));
    table->pml4 = 0; /* PML4 not yet created */
    return table;
}

static void copy_table_level(void* new_table, void* old_table, int level, uint64_t base_virtual_address)
{
    uint64_t* new_entries = (uint64_t*)new_table;
    uint64_t* old_entries = (uint64_t*)old_table;

    // Size each entry maps at this level
    uint64_t entry_size;
    switch (level)
    {
    case 4:
        entry_size = 512ULL * 1024 * 1024 * 1024;
        break; // 512 GiB
    case 3:
        entry_size = 1ULL * 1024 * 1024 * 1024;
        break; //   1 GiB
    case 2:
        entry_size = 2ULL * 1024 * 1024;
        break; //   2 MiB
    case 1:
        entry_size = 4ULL * 1024;
        break; //   4 KiB
    default:
        return;
    }

    for (int i = 0; i < 512; i++)
    {
        uint64_t entry = old_entries[i];
        uint64_t virtual_address = base_virtual_address + (i * entry_size);

        if (!(entry & PAGE_PRESENT)) // Not present
        {
            new_entries[i] = 0;
            continue;
        }

        uint64_t ps_bit_set = entry & PAGE_PS;

        if ((level == 3 && ps_bit_set) || (level == 2 && ps_bit_set) || level == 1)
        {
            // Leaf entry: copy it and optionally mark COW
            uint64_t entry_copy = entry;
            if ((entry & PAGE_WRITABLE) && (entry & PAGE_MASK))
            {
                entry_copy |= PAGE_COW;
                entry_copy &= ~PAGE_WRITABLE;
            }

            new_entries[i] = entry_copy;
        }
        else
        {
            // Non-leaf: recurse into lower level
            void* new_next_level = pages_allocatePage(PAGE_SIZE_4KB);
            memset(new_next_level, 0, PAGE_SIZE_4KB);

            void* old_next_level = (void*)(entry & PAGE_MASK);
            copy_table_level(new_next_level, old_next_level, level - 1, virtual_address);

            uint64_t flags = entry & 0xFFFULL;
            new_entries[i] = ((uint64_t)new_next_level & PAGE_MASK) | flags;
        }
    }
}

page_table_t* pageTable_fork(page_table_t* ref)
{
    uint64_t current_cr3;
    __asm__ volatile("mov %%cr3, %0\n\t" : "=r"(current_cr3) : :);
    __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(KERNEL_PAGE_TABLE->pml4) :);
    page_table_t* table = pageTable_createPageTable();
    if (!table)
        return NULL;

    // Allocate and copy PML4 level (level 4)
    table->pml4 = pages_allocatePage(PAGE_SIZE_4KB);
    if (!table->pml4)
    {
        kfree(table);
        return NULL;
    }
    kmemcpy(table->pml4, ref->pml4, PAGE_SIZE_4KB);

    // Recursively copy page tables starting from PML4 (level 4)
    copy_table_level(table->pml4, ref->pml4, 4, 0);
    __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(current_cr3) :);
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
int pageTable_addPage(page_table_t* pageTable, void* virtual_address, uint64_t page_number, uint64_t page_count, uint64_t pageSize, uint16_t flags)
{
    uint64_t current_cr3;
    __asm__ volatile("mov %%cr3, %0\n\t" : "=r"(current_cr3) : :);
    __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(KERNEL_PAGE_TABLE->pml4) :);
    /* Parameter validation */
    if (!pageTable)
    {
        __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(current_cr3) :);
        return -1;
    }

    /* Initialize PML4 if not present */
    if (!pageTable->pml4)
    {
        pageTable->pml4 = pages_allocatePage(PAGE_SIZE_4KB);
        if (!pageTable->pml4)
            return -1; /* Count not allocate page entry */

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
            {
                __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(current_cr3) :);
                return -1;
            }

            /* Set entry with flags */
            pml4[idx.pml4_index] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITABLE | flags;

            kmemset(pdpt, 0, PAGE_SIZE_4KB);
        }
        else
        {
            /* Existing PDPT - update flags */
            pml4[idx.pml4_index] |= flags;
            pdpt = (uint64_t*)(pml4[idx.pml4_index] & PAGE_MASK);
        }

        /* Handle 1GB pages (PS bit set in PDPT entry) */
        if (pageSize == PAGE_SIZE_1GB)
        {
            pdpt[idx.pdpt_index] = (phys_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_PS | flags;
            continue; /* Skip lower levels */
        }

        /* --- PDPT → PD --- */
        uint64_t* pd;
        if (!(pdpt[idx.pdpt_index] & PAGE_PRESENT))
        {
            /* Allocate new Page Directory */
            pd = pages_allocatePage(PAGE_SIZE_4KB);
            if (!pd)
            {
                __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(current_cr3) :);
                return -1;
            }

            pdpt[idx.pdpt_index] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITABLE | flags;
            kmemset(pd, 0, PAGE_SIZE_4KB);
        }
        else
        {
            pdpt[idx.pdpt_index] |= flags;
            pd = (uint64_t*)(pdpt[idx.pdpt_index] & PAGE_MASK);
        }

        /* Handle 2MB pages (PS bit set in PD entry) */
        if (pageSize == PAGE_SIZE_2MB)
        {
            pd[idx.pd_index] = (phys_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_PS | flags;
            continue; /* Skip lower levels */
        }

        /* --- PD → PT (4KB pages) --- */
        uint64_t* pt;
        if (!(pd[idx.pd_index] & PAGE_PRESENT))
        {
            /* Allocate new Page Table */
            pt = pages_allocatePage(PAGE_SIZE_4KB);
            if (!pt)
            {
                __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(current_cr3) :);
                return -1;
            }

            pd[idx.pd_index] = (uint64_t)pt | PAGE_PRESENT | PAGE_WRITABLE | flags;
            kmemset(pt, 0, PAGE_SIZE_4KB);
        }
        else
        {
            pd[idx.pd_index] |= flags;
            pt = (uint64_t*)(pd[idx.pd_index] & PAGE_MASK);
        }

        /* Set final page table entry */
        pt[idx.pt_index] = (phys_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE | flags;
    }

    __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(current_cr3) :);

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

    /* Initialize PML4 if not present */
    if (!pageTable->pml4)
    {
        pageTable->pml4 = pages_allocatePage(PAGE_SIZE_4KB);
        if (!pageTable->pml4)
            return -1; /* Count not allocate page entry */

        kmemset(pageTable->pml4, 0, PAGE_SIZE_4KB);
    }
    uint64_t current_cr3;
    __asm__ volatile("mov %%cr3, %0\n\t" : "=r"(current_cr3) : :);
    __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(KERNEL_PAGE_TABLE->pml4) :);
    kmemcpy((char*)pageTable->pml4 + 2048, (char*)KERNEL_PAGE_TABLE->pml4 + 2048, 2048);
    __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(current_cr3) :);
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
    __asm__ volatile("invlpg (0)" : ::"memory");

    return 0;
}

page_lookup_result_t pageTable_find_entry(page_table_t* pageTable, uint64_t cr2)
{
    page_lookup_result_t result = {.entry = 0, .size = 0};

    uint64_t* pml4 = pageTable->pml4;

    page_table_indices_t indices = extract_indices(cr2);

    uint64_t pml4e = pml4[indices.pml4_index];
    if (!(pml4e & PAGE_PRESENT))
        return result;

    uint64_t* pdpt = (uint64_t*)(pml4e & PAGE_MASK);
    uint64_t pdpte = pdpt[indices.pdpt_index];
    if (!(pdpte & PAGE_PRESENT))
        return result;

    // Check for 1GiB page
    if (pdpte & PAGE_PS)
    {
        result.entry = pdpt[indices.pdpt_index];
        result.size = PAGE_SIZE_1GB;
        return result;
    }

    uint64_t* pd = (uint64_t*)(pdpte & PAGE_MASK);
    uint64_t pde = pd[indices.pd_index];
    if (!(pde & PAGE_PRESENT))
        return result;

    // Check for 2MiB page
    if (pde & PAGE_PS)
    {
        result.entry = pd[indices.pd_index];
        result.size = PAGE_SIZE_2MB;
        return result;
    }

    uint64_t* pt = (uint64_t*)(pde & PAGE_MASK);
    uint64_t pte = pt[indices.pt_index];
    if (!(pte & PAGE_PRESENT))
        return result;

    result.entry = pt[indices.pt_index];
    result.size = PAGE_SIZE_4KB;
    return result;
}