/**
 * @file gdt.c
 * @brief Global Descriptor Table (GDT) Setup
 *
 * Sets up the x86_64 GDT, TSS, and loads segment registers for kernel and user mode.
 */
#include <arch/gdt.h>

#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <memory/memoryMap.h>

#define GDT_ENTRIES 7

void KERNEL_InitGDT()
{
    GDTEntry* gdt = (GDTEntry*)kmalloc(sizeof(GDTEntry) * GDT_ENTRIES);
    kmemset(gdt, 0, sizeof(GDTEntry) * GDT_ENTRIES);
    TSS64* tss = TSS;
    kmemset(tss, 0, sizeof(TSS64));

    // Setup kernel stack (must be mapped in user space page tables with supervisor-only permission)
    tss->rsp0 = 0x00000037ffff0000; //(uint64_t)KERNEL_STACK_START + (uint64_t)KERNEL_STACK_SIZE;

    // Optionally set IST[0] if you use it (like for #DF or NMI)
    // tss->ist1 = (uint64_t)some_emergency_stack;

    // GDT Entries
    gdt[0] = (GDTEntry){0}; // Null descriptor

    gdt[1] = (GDTEntry){
        .access = 0x9A, // Kernel code
        .granularity = 0x20,
    };

    gdt[2] = (GDTEntry){
        .access = 0x92, // Kernel data
    };

    gdt[3] = (GDTEntry){
        .access = 0xFA, // User code
        .granularity = 0x20,
    };

    gdt[4] = (GDTEntry){
        .access = 0xF2, // User data
    };

    // TSS Descriptor (needs 2 entries: GDT[5] and GDT[6])
    uint64_t base = (uint64_t)tss;
    uint16_t limit = sizeof(TSS64) - 1;

    gdt[5] = (GDTEntry){
        .limit_low = limit & 0xFFFF,
        .base_low = base & 0xFFFF,
        .base_mid = (base >> 16) & 0xFF,
        .access = 0x89, // Present, type = 0x9 (Available 64-bit TSS)
        .granularity = ((limit >> 16) & 0x0F),
        .base_high = (base >> 24) & 0xFF,
    };

    // High 32 bits of TSS base go into the next 8 bytes
    ((uint32_t*)&gdt[6])[0] = (base >> 32) & 0xFFFFFFFF;
    ((uint32_t*)&gdt[6])[1] = 0;

    GDTPtr gdt_ptr = {.limit = sizeof(GDTEntry) * GDT_ENTRIES - 1, .base = (uint64_t)gdt};

    // Load GDT and set segment registers
    __asm__ volatile("lgdt %0\n\t"
                     "mov $0x10, %%ax\n\t"
                     "mov %%ax, %%ds\n\t"
                     "mov %%ax, %%es\n\t"
                     "mov %%ax, %%fs\n\t"
                     "mov %%ax, %%gs\n\t"
                     "mov %%ax, %%ss\n\t"
                     "pushq $0x08\n\t"
                     "leaq 1f(%%rip), %%rax\n\t"
                     "pushq %%rax\n\t"
                     "lretq\n\t"
                     "1:"
                     :
                     : "m"(gdt_ptr)
                     : "rax", "memory");

    // Load the TSS (selector = 0x28 = GDT index 5 << 3)
    __asm__ volatile("ltr %%ax" : : "a"(0x28));
}