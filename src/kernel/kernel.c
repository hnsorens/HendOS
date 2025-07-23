/**
 * @file kernel.c
 * @brief Kernel Main Entry Point and Initialization
 *
 * Handles the transition from UEFI bootloader to kernel mode, sets up critical
 * subsystems (memory, devices, processes), and starts the first user process.
 */

#include "efibind.h"
#include <arch/gdt.h>
#include <arch/idt.h>
#include <arch/io.h>
#include <boot/bootServices.h>
#include <boot/elfLoader.h>
#include <drivers/fbcon.h>
#include <drivers/vcon.h>
#include <efi.h>
#include <efilib.h>
#include <fs/fontLoader.h>
#include <fs/vfs.h>
#include <kernel/device.h>
#include <kernel/pidHashTable.h>
#include <kernel/scheduler.h>
#include <kmath.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <memory/kpool.h>
#include <memory/memoryMap.h>
#include <memory/paging.h>
#include <misc/debug.h>
#include <kernel/syscalls.h>
#include <stdint.h>

/* ==================== Forward Declarations ==================== */

static EFI_STATUS init_framebuffer(preboot_info_t* preboot_info, EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table);
static uint64_t calculate_total_system_memory(preboot_info_t* preboot_info);
static void init_clock(void);
static void setup_kernel_mappings(page_table_t* kernel_pt, uint64_t* early_allocations);
static void find_kernel_memory(void);
static void reserve_kernel_memory(uint64_t total_memory_size);
static void init_subsystems(void);
static void launch_system_processes(void);
static void* alloc_kernel_memory(size_t page_count);
static int pageTable_addKernelPage(page_table_t* pageTable, void* virtual_address, uint64_t page_number, uint64_t page_count, uint64_t pageSize, uint64_t* early_allocations);

/* ==================== Main Entry Point ==================== */

MemoryRegion regions[] = {
    {0, KERNEL_HEAP_SIZE},  /* HEAP      */
    {0, KERNEL_STACK_SIZE}, /* STACK     */
    {0, PAGE_ALLOCATION_TABLE_SIZE},
    /* Page AllocateTable*/ // DONE WITH THIS ONE
    {0, PAGE_TABLE_SIZE},
    /* palceholder Page table */ // DONE WITH THIS ONE
    {0, GLOBAL_VARS_SIZE},       /* Global variables*/
    {0, 0}                       /* Framebuffer (allready allocated) */
};

/* Font object is too big for stack */
font_t TEMPFONT;

/* Needs to be accessed in other functions */
preboot_info_t preboot_info;

/**
 * @brief UEFI Entry Point - Transitions from bootloader to kernel
 * @param ImageHandle EFI handle for the loaded image
 * @param SystemTable EFI system table pointer
 * @return EFI_STATUS Always returns EFI_SUCCESS if kernel runs
 *
 * Execution Phases:
 * 1. Early UEFI environment setup
 * 2. Memory management initialization
 * 3. Critical hardware initialization
 * 4. Process creation and scheduling start
 */
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
    /* Initializes EFI Services */
    InitializeLib(ImageHandle, SystemTable);

    Print(u"STARTING!\n");

    /* =============== COLLECT SYSTEM INFORMATION =============== */
    if (EFI_ERROR(init_framebuffer(&preboot_info, ImageHandle, SystemTable)))
    {
        Print(u"Framebuffer Initialization Failed!\n");
        return EFI_LOAD_ERROR;
    }

    /* Load terminal font before exiting boot services */
    font_init(&TEMPFONT, u"UbuntuMono-Regular.ttf", 20, ImageHandle);

    /* Step 2: Exit UEFI environment */
    if (EFI_ERROR(exit_boot_services(&preboot_info, ImageHandle, SystemTable)))
    {
        Print(u"Failed to exit boot services!\n");
        return EFI_LOAD_ERROR;
    }

    /* =============== MEMORY MANAGEMENT SETUP =============== */
    find_kernel_memory();

    /* Early allocations to catch allocations made before allocation table */
    uint64_t* early_allocations = alloc_kernel_memory(512); /* 2 gb */
    early_allocations[0] = 0;

    /* Get total memory and build kernel page table */
    uint64_t total_memory = calculate_total_system_memory(&preboot_info);
    page_table_t kernel_page_table;
    kernel_page_table = alloc_kernel_memory(1);
    early_allocations[++early_allocations[0]] = (uint64_t)kernel_page_table;

    /* Zero out the PML4 table */
    kmemset(kernel_page_table, 0, PAGE_SIZE_4KB);

    /* Add all kernel pdpt entries */
    for (int i = 0; i < 512; i++)
    {
        void* page = alloc_kernel_memory(1);
        early_allocations[++early_allocations[0]] = (uint64_t)page;
        kernel_page_table[i] = (uint64_t)page | PAGE_WRITABLE | PAGE_PRESENT;
    }

    /* Identity map all physical memory */
    pageTable_addKernelPage(&kernel_page_table, 0,        /* Virtual = Physical */
                            0,                            /* Start at physical 0 */
                            total_memory / PAGE_SIZE_4KB, /* Number of 4KB pages */
                            PAGE_SIZE_4KB, early_allocations);

    setup_kernel_mappings(&kernel_page_table, early_allocations);
    pageTable_set(kernel_page_table);

    /* Copy global variables that need to stay after kernel jump */
    kmemset((void*)GLOBAL_VARS_START, 0, GLOBAL_VARS_SIZE);
    kmemcpy(INTEGRATED_FONT, &TEMPFONT, sizeof(font_t));
    kmemcpy(MEMORY_REGIONS, regions, sizeof(regions));
    kmemcpy(PREBOOT_INFO, &preboot_info, sizeof(preboot_info_t));

    /* =============== CRITICAL MEMORY REGIONS =============== */
    reserve_kernel_memory(total_memory);
    pages_generateFreeStack();

    (*KERNEL_PAGE_TABLE) = kernel_page_table;

    kinitHeap((void*)KERNEL_HEAP_START, KERNEL_HEAP_SIZE);
    /* =============== TRANSITION TO KERENL MODE =============== */

    /* Initialize Stack Memory */
    __asm__ volatile("mov %0, %%rsp\n\t" : : "r"(KERNEL_STACK_START + KERNEL_STACK_SIZE - 4096) :);
    /* Jump to kernel code */
    __asm__ volatile("mov %[kernel_code], %%r9\n\t"
                     "lea 0f(%%rip), %%rax\n\t"
                     "add %%rax, %%r9\n\t"
                     "jmp *%%r9\n\t"
                     "0:\n\t"
                     :
                     : [kernel_code] "r"(KERNEL_CODE_START)
                     : "rax", "rcx", "r9", "r8");

    /* =============== KERNEL MODE NOW ACTIVE =============== */

    /* Initialize Subsystems */
    init_subsystems();

    /* Launch the System Processes */
    launch_system_processes();

    return EFI_LOAD_ERROR;
}

/* ==================== Initialization Functions ==================== */

static void find_kernel_memory()
{
    uint32_t regions_count = sizeof(regions) / sizeof(MemoryRegion);
    UINTN numRegions = preboot_info.MemoryMapSize / preboot_info.DescriptorSize;
    EFI_MEMORY_DESCRIPTOR* entry = preboot_info.MemoryMap;
    uint64_t max = 0;
    for (UINTN i = 0; i < numRegions; i++)
    {
        /* Only consider conventional memory as free */
        if (entry->Type == EfiConventionalMemory)
        {
            for (size_t j = 0; j < regions_count; j++)
            {
                if (regions[j].base == 0 && entry->NumberOfPages * 4096 >= regions[j].size)
                {
                    regions[j].base = entry->PhysicalStart;
                    entry->PhysicalStart += regions[j].size;
                    entry->NumberOfPages -= regions[j].size / 4096;
                }
            }
        }
        entry = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)entry + preboot_info.DescriptorSize);
    }
}

static void* alloc_kernel_memory(size_t page_count)
{
    UINTN numRegions = preboot_info.MemoryMapSize / preboot_info.DescriptorSize;
    EFI_MEMORY_DESCRIPTOR* entry = preboot_info.MemoryMap;
    uint64_t max = 0;
    for (UINTN i = 0; i < numRegions; i++)
    {
        /* Only consider conventional memory as free */
        if (entry->Type == EfiConventionalMemory)
        {
            if (entry->NumberOfPages > 0 && entry->NumberOfPages >= page_count)
            {
                void* start = (void*)entry->PhysicalStart;
                entry->PhysicalStart += page_count * PAGE_SIZE_4KB;
                entry->NumberOfPages -= page_count;
                return start;
            }
        }
        entry = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)entry + preboot_info.DescriptorSize);
    }
    return 0;
}

/**
 * @brief Initialize framebuffer graphics output
 * @param info Preboot info structure to populate
 * @param imageHandle UEFI image handle
 * @param systemTable UEFI system table
 * @return EFI_STATUS Success/failure code
 */
static EFI_STATUS init_framebuffer(preboot_info_t* preboot_info, EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE* systemTable)
{
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    status = systemTable->BootServices->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&(gop));

    if (!EFI_ERROR(status))
    {
        preboot_info->screen_width = gop->Mode->Info->HorizontalResolution;
        preboot_info->screen_height = gop->Mode->Info->VerticalResolution;
        preboot_info->framebuffer_size = gop->Mode->FrameBufferSize;
        preboot_info->framebuffer = (uint32_t*)gop->Mode->FrameBufferBase;
    }
    return status;
}

/**
 * @brief Configure PIT for system timing
 */
static void init_clock(void)
{
    /* Configure PIT channel 0 for 50Hz interrupts */
    uint16_t divisor = 1193 * 50; /* 1.193 MHz / 1000 = 1ms */
    outb(0x43, 0x36);             /* Command port: Channel 0, mode 3 */
    outb(0x40, divisor & 0xFF);   /* Low Byte */
    outb(0x40, divisor >> 8);     /* High Byte */
}

/**
 * @brief Setup virtual memory mappings for kernel
 * @param kernel_pt Kernel page table to populate
 */
static void setup_kernel_mappings(page_table_t* kernel_pt, uint64_t* early_allocations)
{
    /* Map EFI memory regions */
    UINTN numRegions = preboot_info.MemoryMapSize / preboot_info.DescriptorSize;
    EFI_MEMORY_DESCRIPTOR* entry = preboot_info.MemoryMap;

    for (UINTN i = 0; i < numRegions; i++)
    {
        if (entry->Type != EfiConventionalMemory)
        {
            uint64_t start_4kb = entry->PhysicalStart / PAGE_SIZE_4KB;
            uint64_t count_4kb = entry->NumberOfPages;
            pageTable_addKernelPage(kernel_pt, (void*)(entry->PhysicalStart + KERNEL_CODE_START), start_4kb, count_4kb, PAGE_SIZE_4KB, early_allocations);
        }
        entry = (EFI_MEMORY_DESCRIPTOR*)((uint8_t*)entry + preboot_info.DescriptorSize);
    }

    /* Map kernel specific regions */

    /* KernelHeap */
    pageTable_addKernelPage(kernel_pt, (void*)KERNEL_HEAP_START, regions[0].base / PAGE_SIZE_4KB, regions[0].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB, early_allocations);

    /* Kernel Stack */
    pageTable_addKernelPage(kernel_pt, (void*)KERNEL_STACK_START, regions[1].base / PAGE_SIZE_4KB, regions[1].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB, early_allocations);

    /* Page Allocation Table */
    pageTable_addKernelPage(kernel_pt, (void*)PAGE_ALLOCATION_TABLE_START, regions[2].base / PAGE_SIZE_4KB, regions[2].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB, early_allocations);

    /* Global Variables */
    pageTable_addKernelPage(kernel_pt, (void*)GLOBAL_VARS_START, regions[4].base / PAGE_SIZE_4KB, regions[4].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB, early_allocations);

    /* Framebuffer */
    pageTable_addKernelPage(kernel_pt, (void*)FRAMEBUFFER_START, (uint64_t)preboot_info.framebuffer / PAGE_SIZE_4KB, preboot_info.framebuffer_size / PAGE_SIZE_4KB, PAGE_SIZE_4KB, early_allocations);
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
static int pageTable_addKernelPage(page_table_t* pageTable, void* virtual_address, uint64_t page_number, uint64_t page_count, uint64_t pageSize, uint64_t* early_allocations)
{
    /* Parameter validation */
    if (!pageTable)
    {
        return -1;
    }

    uint64_t vaddr = (uint64_t)virtual_address;
    uint64_t* pml4 = *pageTable;

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
            pdpt = alloc_kernel_memory(1);
            early_allocations[++early_allocations[0]] = (uint64_t)pdpt;
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
            pd = alloc_kernel_memory(1);
            early_allocations[++early_allocations[0]] = (uint64_t)pd;
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
            // pt = (uint64_t*)((uint64_t)pml4 + pageTable->size);
            pt = alloc_kernel_memory(1);
            early_allocations[++early_allocations[0]] = (uint64_t)pt;
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
 * @brief Calculate the total memory on the system
 */
static uint64_t calculate_total_system_memory(preboot_info_t* preboot_info)
{
    /* Map EFI memory regions */
    UINTN numRegions = preboot_info->MemoryMapSize / preboot_info->DescriptorSize;
    EFI_MEMORY_DESCRIPTOR* entry = preboot_info->MemoryMap;
    uint64_t max = 0;

    /* Adds size of all regions*/
    for (UINTN i = 0; i < numRegions; i++)
    {
        uint64_t region_end = entry->PhysicalStart + entry->NumberOfPages * 4096;
        if (region_end > max)
            max = region_end;

        entry = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)entry + preboot_info->DescriptorSize);
    }

    return 17179869184;
}

/**
 * @brief Reserve physical memory for kernel use
 */
static void reserve_kernel_memory(uint64_t total_memory_size)
{
    /* Calculate total number of pages */
    *NUM_2MB_PAGES = total_memory_size / PAGE_SIZE_2MB;
    *NUM_4KB_PAGES = total_memory_size / PAGE_SIZE_4KB;

    size_t alloc_table_total_size = (*NUM_2MB_PAGES / 64) + (*NUM_4KB_PAGES / 64) + ((*NUM_2MB_PAGES + *NUM_4KB_PAGES) * sizeof(uint32_t));
    void* page_allocation_table = alloc_kernel_memory(ALIGN_UP(alloc_table_total_size, 4096) / 4096);

    /* Initialize Pages Allocate Table*/
    pages_initAllocTable((void*)PAGE_ALLOCATION_TABLE_START, total_memory_size, MEMORY_REGIONS, sizeof(regions) / sizeof(MemoryRegion));

    /* Reserve allocation table */
    pages_reservePage((uint64_t)page_allocation_table / PAGE_SIZE_4KB, ALIGN_UP(alloc_table_total_size, 4096) / 4096, PAGE_SIZE_4KB);

    /* Set page table memory with framebuffer */
    MEMORY_REGIONS[5].base = (uint64_t)preboot_info.framebuffer;
    MEMORY_REGIONS[5].size = (uint64_t)preboot_info.framebuffer_size;

    /* Reserve all non-conventional memory regions */
    UINTN numRegions = preboot_info.MemoryMapSize / preboot_info.DescriptorSize;
    EFI_MEMORY_DESCRIPTOR* entry = preboot_info.MemoryMap;

    for (UINTN i = 0; i < numRegions; i++)
    {
        if (entry->Type != EfiConventionalMemory)
        {
            pages_reservePage(entry->PhysicalStart / PAGE_SIZE_4KB, entry->NumberOfPages, PAGE_SIZE_4KB);
        }
        entry = (EFI_MEMORY_DESCRIPTOR*)((uint8_t*)entry + preboot_info.DescriptorSize);
    }

    /* Reserve kernel-specific regions */

    /* Kernel Heap */
    pages_reservePage(MEMORY_REGIONS[0].base / PAGE_SIZE_4KB, KERNEL_HEAP_SIZE / PAGE_SIZE_4KB, PAGE_SIZE_4KB);

    /* Kernel Heap */
    pages_reservePage(MEMORY_REGIONS[1].base / PAGE_SIZE_4KB, KERNEL_STACK_SIZE / PAGE_SIZE_4KB, PAGE_SIZE_4KB);

    /* Page Allocation Table */
    pages_reservePage(MEMORY_REGIONS[2].base / PAGE_SIZE_4KB, PAGE_ALLOCATION_TABLE_SIZE / PAGE_SIZE_4KB, PAGE_SIZE_4KB);

    /* Global Variables */
    pages_reservePage(MEMORY_REGIONS[4].base / PAGE_SIZE_4KB, GLOBAL_VARS_SIZE / PAGE_SIZE_4KB, PAGE_SIZE_4KB);

    /* Framebuffer */
    pages_reservePage(MEMORY_REGIONS[5].base / PAGE_SIZE_4KB, FRAMEBUFFER_SIZE / PAGE_SIZE_4KB, PAGE_SIZE_4KB);
}

/**
 * @brief Initialize kernel subsystems
 */
static void init_subsystems(void)
{
    void* page = pages_allocatePage(PAGE_SIZE_2MB);
    *TEMP_MEMORY = (void*)0xFFFFB40000000000; /* 180tb */
    pageTable_addPage(KERNEL_PAGE_TABLE, (void*)0xFFFFB40000000000, (uint64_t)page / PAGE_SIZE_2MB, 1, PAGE_SIZE_2MB, 0);

    *PROCESS_POOL = pool_create(sizeof(process_t), 16);
    *INODE_POOL = pool_create(sizeof(ext2_inode), 8);
    *VFS_ENTRY_POOL = pool_create(sizeof(vfs_entry_t), 8);
    *OPEN_FILE_POOL = pool_create(sizeof(file_descriptor_t), 8);
    *PROCESS_GROUP_POOL = pool_create(sizeof(process_group_t), 8);
    *SESSION_POOL = pool_create(sizeof(process_session_t), 8);
    *FD_ENTRY_POOL = pool_create(sizeof(file_descriptor_entry_t), 8);

    init_clock();
    vfs_init();
    keyboard_init();
    mouse_init();

    KERNEL_InitGDT();
    KERNEL_InitIDT();
    syscall_init();

    /* Graphics initialization */
    GRAPHICS_InitGraphics(kmalloc(sizeof(uint32_t) * 1920 * 1080));

    /* Terminal initialization */
    vcon_init();
    fbcon_init();

    pid_hash_init(PID_MAP, (void*)0xFFFF8D0000000000);
    pid_hash_init(PGID_MAP, (void*)0xFFFF8E0000000000);
    pid_hash_init(SID_MAP, (void*)0xFFFF8F0000000000);
}

/**
 * @brief Launch the userspace processes (systemd)
 */
static void launch_system_processes(void)
{
    /* Launch Systemd Process */

    vfs_entry_t* entry;
    vfs_find_entry(ROOT, &entry, "bin/systemd");
    if (entry && entry->type == EXT2_FT_REG_FILE)
    {
        file_descriptor_t* open_file = fdm_open_file(entry);
        elfLoader_systemd(open_file);
    }
    (*CURRENT_PROCESS) = scheduler_nextProcess();
    /* Switch page table to process */
    __asm__ volatile("mov %0, %%cr3\n\t" : : "r"((*CURRENT_PROCESS)->page_table) :);

    /* Switch to process stack signature */
    __asm__ volatile("mov %0, %%rsp\n\t" : : "r"(&(*CURRENT_PROCESS)->process_stack_signature) :);

    TSS->ist1 = (uint64_t)(*CURRENT_PROCESS) + sizeof(process_stack_layout_t);

    /* pop all registers */
    __asm__ volatile("mov $0x23, %%ax\n\t"
                     "mov %%ax, %%ds\n\t"
                     "mov %%ax, %%es\n\t"
                     "pop %%r15\n\t"
                     "pop %%r14\n\t"
                     "pop %%r13\n\t"
                     "pop %%r12\n\t"
                     "pop %%r11\n\t"
                     "pop %%r10\n\t"
                     "pop %%r9\n\t"
                     "pop %%r8\n\t"
                     "pop %%rbp\n\t"
                     "pop %%rdi\n\t"
                     "pop %%rsi\n\t"
                     "pop %%rdx\n\t"
                     "pop %%rcx\n\t"
                     "pop %%rbx\n\t"
                     "pop %%rax\n\t"
                     :
                     :
                     :);
    __asm__ volatile("iretq\n\t" : ::);
}