/**
 * @file kernel.c
 * @brief Kernel Main Entry Point and Initialization
 *
 * Handles the transition from UEFI bootloader to kernel mode,
 * sets up critical subsystems, and starts the first process.
 */

#include <arch/gdt.h>
#include <arch/idt.h>
#include <arch/io.h>
#include <boot/bootServices.h>
#include <drivers/fbcon.h>
#include <drivers/vcon.h>
#include <efi.h>
#include <efilib.h>
#include <fs/filesystem.h>
#include <fs/fontLoader.h>
#include <kernel/device.h>
#include <kernel/shell.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <memory/memoryMap.h>
#include <memory/paging.h>
#include <misc/debug.h>

/* ==================== Forward Declarations ==================== */

static EFI_STATUS init_framebuffer(preboot_info_t* preboot_info,
                                   EFI_HANDLE image_handle,
                                   EFI_SYSTEM_TABLE* system_table);
static uint64_t calculate_total_system_memory(preboot_info_t* preboot_info);
static void init_clock(void);
static void setup_kernel_mappings(page_table_t* kernel_pt);
static void find_kernel_memory(void);
static void reserve_kernel_memory(uint64_t total_memory_size);
static void init_subsystems(void);
static void launch_system_processes(void);

/* ==================== Main Entry Point ==================== */

MemoryRegion regions[] = {
    {0, KERNEL_HEAP_SIZE},           /* HEAP      */
    {0, KERNEL_STACK_SIZE},          /* STACK     */
    {0, PAGE_ALLOCATION_TABLE_SIZE}, /* Page AllocateTable*/
    {0, PAGE_TABLE_SIZE},            /* palceholder Page table */
    {0, GLOBAL_VARS_SIZE},           /* Global variables*/
    {0, 0}                           /* Framebuffer (allready allocated) */
};

/* Font object is too big for stack */
Font TEMPFONT;

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

    /* =============== COLLECT SYSTEM INFORMATION =============== */
    if (EFI_ERROR(init_framebuffer(&preboot_info, ImageHandle, SystemTable)))
    {
        Print(L"Framebuffer Initialization Failed!\n");
        return EFI_LOAD_ERROR;
    }

    /* Load terminal font before exiting boot services */
    KERNEL_InitTerminalFont(&TEMPFONT, L"UbuntuMono-Regular.ttf", 20, ImageHandle);

    /* Step 2: Exit UEFI environment */
    if (EFI_ERROR(KERNEL_ExitBootService(&preboot_info, ImageHandle, SystemTable)))
    {
        Print(L"Failed to exit boot services!\n");
        return EFI_LOAD_ERROR;
    }

    /* =============== MEMORY MANAGEMENT SETUP =============== */
    find_kernel_memory();

    /* Get total memory and build kernel page table */
    uint64_t total_memory = calculate_total_system_memory(&preboot_info);
    page_table_t kernel_page_table =
        pageTable_createKernelPageTable((void*)regions[3].base, total_memory);

    setup_kernel_mappings(&kernel_page_table);
    pageTable_set((void*)regions[3].base);

    /* Copy global variables that need to stay after kernel jump */
    kmemset(GLOBAL_VARS_START, 0, GLOBAL_VARS_SIZE);
    kmemcpy(INTEGRATED_FONT, &TEMPFONT, sizeof(Font));
    kmemcpy(MEMORY_REGIONS, regions, sizeof(regions));
    kmemcpy(PREBOOT_INFO, &preboot_info, sizeof(preboot_info_t));

    /* =============== CRITICAL MEMORY REGIONS =============== */
    kinitHeap(KERNEL_HEAP_START, KERNEL_HEAP_SIZE);
    reserve_kernel_memory(total_memory);
    pages_generateFreeStack();

    /* =============== TRANSITION TO KERENL MODE =============== */

    (*KERNEL_PAGE_TABLE) = kernel_page_table;

    /* Initialize Stack Memory */
    __asm__ volatile("mov %0, %%rsp\n\t" : : "r"(KERNEL_STACK_START + KERNEL_STACK_SIZE - 4096) :);
    /* Jump to kernel code */
    __asm__ volatile("mov %[kernel_code], %%r9\n\t"
                     "lea 0f(%%rip), %%rax\n\t"
                     "add %%rax, %%r9\n\t"
                     "jmp %%r9\n\t"
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
    return EFI_SUCCESS;
}

/**
 * @brief Initialize framebuffer graphics output
 * @param info Preboot info structure to populate
 * @param imageHandle UEFI image handle
 * @param systemTable UEFI system table
 * @return EFI_STATUS Success/failure code
 */
static EFI_STATUS init_framebuffer(preboot_info_t* preboot_info,
                                   EFI_HANDLE imageHandle,
                                   EFI_SYSTEM_TABLE* systemTable)
{
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    status = systemTable->BootServices->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL,
                                                       (VOID**)&(gop));

    if (!EFI_ERROR(status))
    {
        preboot_info->screen_width = gop->Mode->Info->HorizontalResolution;
        preboot_info->screen_height = gop->Mode->Info->VerticalResolution;
        preboot_info->framebuffer_size = gop->Mode->FrameBufferSize;
        preboot_info->framebuffer = gop->Mode->FrameBufferBase;
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
static void setup_kernel_mappings(page_table_t* kernel_pt)
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
            pageTable_addKernelPage(kernel_pt, entry->PhysicalStart + KERNEL_CODE_START, start_4kb,
                                    count_4kb, PAGE_SIZE_4KB);
        }
        entry = (EFI_MEMORY_DESCRIPTOR*)((uint8_t*)entry + preboot_info.DescriptorSize);
    }

    /* Map kernel specific regions */

    /* KernelHeap */
    pageTable_addKernelPage(kernel_pt, KERNEL_HEAP_START, regions[0].base / PAGE_SIZE_4KB,
                            regions[0].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB);

    /* Kernel Stack */
    pageTable_addKernelPage(kernel_pt, KERNEL_STACK_START, regions[1].base / PAGE_SIZE_4KB,
                            regions[1].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB);

    /* Page Allocation Table */
    pageTable_addKernelPage(kernel_pt, PAGE_ALLOCATION_TABLE_START, regions[2].base / PAGE_SIZE_4KB,
                            regions[2].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB);

    /* Kernel Page Table */
    pageTable_addKernelPage(kernel_pt, PAGE_TABLE_START, regions[3].base / PAGE_SIZE_4KB,
                            regions[3].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB);

    /* Global Variables */
    pageTable_addKernelPage(kernel_pt, GLOBAL_VARS_START, regions[4].base / PAGE_SIZE_4KB,
                            regions[3].size / PAGE_SIZE_4KB, PAGE_SIZE_4KB);

    /* Framebuffer */
    pageTable_addKernelPage(kernel_pt, FRAMEBUFFER_START,
                            (uint64_t)preboot_info.framebuffer / PAGE_SIZE_4KB,
                            preboot_info.framebuffer_size / PAGE_SIZE_4KB, PAGE_SIZE_4KB);
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
    /* Initialize Pages Allocate Table*/
    pages_initAllocTable(PAGE_ALLOCATION_TABLE_START, total_memory_size, MEMORY_REGIONS,
                         sizeof(regions) / sizeof(MemoryRegion));

    /* Set page table memory with framebuffer */
    MEMORY_REGIONS[5].base = preboot_info.framebuffer;
    MEMORY_REGIONS[5].size = preboot_info.framebuffer_size;

    /* Reserve all non-conventional memory regions */
    UINTN numRegions = preboot_info.MemoryMapSize / preboot_info.DescriptorSize;
    EFI_MEMORY_DESCRIPTOR* entry = preboot_info.MemoryMap;

    for (UINTN i = 0; i < numRegions; i++)
    {
        if (entry->Type != EfiConventionalMemory)
        {
            pages_reservePage(entry->PhysicalStart / PAGE_SIZE_4KB, entry->NumberOfPages,
                              PAGE_SIZE_4KB);
        }
        entry = (EFI_MEMORY_DESCRIPTOR*)((uint8_t*)entry + preboot_info.DescriptorSize);
    }

    /* Reserve kernel-specific regions */

    /* Kernel Heap */
    pages_reservePage(MEMORY_REGIONS[0].base / PAGE_SIZE_4KB, KERNEL_HEAP_SIZE / PAGE_SIZE_4KB,
                      PAGE_SIZE_4KB);

    /* Kernel Heap */
    pages_reservePage(MEMORY_REGIONS[1].base / PAGE_SIZE_4KB, KERNEL_STACK_SIZE / PAGE_SIZE_4KB,
                      PAGE_SIZE_4KB);

    /* Page Allocation Table */
    pages_reservePage(MEMORY_REGIONS[2].base / PAGE_SIZE_4KB,
                      PAGE_ALLOCATION_TABLE_SIZE / PAGE_SIZE_4KB, PAGE_SIZE_4KB);

    /* Kernel Page Table */
    pages_reservePage(MEMORY_REGIONS[3].base / PAGE_SIZE_4KB, PAGE_TABLE_SIZE / PAGE_SIZE_4KB,
                      PAGE_SIZE_4KB);

    /* Global Variables */
    pages_reservePage(MEMORY_REGIONS[4].base / PAGE_SIZE_4KB, GLOBAL_VARS_SIZE / PAGE_SIZE_4KB,
                      PAGE_SIZE_4KB);

    /* Framebuffer */
    pages_reservePage(MEMORY_REGIONS[5].base / PAGE_SIZE_4KB, FRAMEBUFFER_SIZE / PAGE_SIZE_4KB,
                      PAGE_SIZE_4KB);
}

/**
 * @brief Initialize kernel subsystems
 */
static void init_subsystems(void)
{
    init_clock();
    filesystem_init();
    dev_init();
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

    /* Process memory management */
    for (int i = 0; i < 2048; i++)
    {
        PROCESS_MEM_FREE_STACK[i + 1] = 2048 - i;
    }
    PROCESS_MEM_FREE_STACK[0] = 2048;
}

/**
 * @brief Launch the userspace processes (systemd)
 */
static void launch_system_processes(void)
{
    /* Launch Systemd Process */
    directory_t* directory;
    filesystem_findDirectory(ROOT, &directory, "bin");
    for (int i = 0; i < directory->entry_count; i++)
    {
        filesystem_entry_t* entry = directory->entries[i];
        if (entry->file_type == EXT2_FT_REG_FILE && kernel_strcmp(entry->file.name, "systemd") == 0)
        {
            page_table_t* table = pageTable_createPageTable();
            elfLoader_load(table, 0, &entry->file.file);
        }
    }

    if (*PROCESSES)
    {
        __asm__ volatile("mov %%r11, %0\n\t" : "=r"((*CURRENT_PROCESS)->stackPointer)::);

        (*CURRENT_PROCESS) = scheduler_nextProcess();
        /* Switch page table to process */
        __asm__ volatile("mov %0, %%cr3\n\t" ::"r"((*CURRENT_PROCESS)->page_table->pml4) :);
        /* Switch to process stack signature */
        __asm__ volatile("mov %0, %%rsp\n\t" ::"r"(&(*CURRENT_PROCESS)->process_stack_signature) :);

        // TODO: set the TTS rp1 to the data
        TSS->ist1 = &(*CURRENT_PROCESS)->process_stack_signature + sizeof(process_stack_layout_t);
        LOG_VARIABLE(&(*CURRENT_PROCESS)->process_stack_signature + sizeof(process_stack_layout_t),
                     "r15");
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
                         "pop %%rax\n\t" ::
                             :);

        __asm__ volatile("iretq\n\t" :::);
    }
}