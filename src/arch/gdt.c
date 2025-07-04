/* gdt.c */
/**
 * @file gdt.c
 * @brief Global Descriptor Table Implementation
 *
 * Implements GDT initialization and loading for x86-64 architecture
 */

#include <arch/gdt.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <memory/memoryMap.h>

/**
 * @brief Initializes the Global Descriptor Table and Task State Segment
 *
 * Sets up kernel and user code/data segments, initializes TSS,
 * and loads them into the processor
 */
void gdt_init()
{
    /* Clear GDT and TSS memory */
    GDTEntry* gdt = GDT;
    memset(gdt, 0, sizeof(GDTEntry) * GDT_ENTRIES);
    TSS64* tss = TSS;
    memset(tss, 0, sizeof(TSS64));

    /* Setup kernel stack pointer for ring 0 */
    tss->rsp0 = 0x00000037ffff0000;

    /* Null descriptor (required) */
    gdt[0] = (GDTEntry){0};

    /* Kernel code segment descriptor */
    gdt[1] = (GDTEntry){
        .access = 0x9A,      ///< Present, Ring 0, Code segment, Exec/Read
        .granularity = 0x20, ///< 64-bit mode, limit high bits
    };

    /* Kernel data segment descriptor */
    gdt[2] = (GDTEntry){
        .access = 0x92, ///< Present, Ring 0, Data segment, Read/Write
    };

    /* User code segment descriptor */
    gdt[3] = (GDTEntry){
        .access = 0xFA,      ///< Present, Ring 3, Code segment, Exec/Read
        .granularity = 0x20, ///< 64-bit mode, limit high bits
    };

    /* User data segment descriptor */
    gdt[4] = (GDTEntry){
        .access = 0xF2, ///< Present, Ring 3, Data segment, Read/Write
    };

    /* TSS descriptor (spans two GDT entries) */
    uint64_t base = (uint64_t)tss;
    uint16_t limit = sizeof(TSS64) - 1;

    /* First part of TSS descriptor */
    gdt[5] = (GDTEntry){
        .limit_low = limit & 0xFFFF,
        .base_low = base & 0xFFFF,
        .base_mid = (base >> 16) & 0xFF,
        .access = 0x89, ///< Present, 64-bit TSS (Available)
        .granularity = ((limit >> 16) & 0x0F),
        .base_high = (base >> 24) & 0xFF,
    };

    /* Second part of TSS descriptor (high 32 bits of base) */
    ((uint32_t*)&gdt[6])[0] = (base >> 32) & 0xFFFFFFFF;
    ((uint32_t*)&gdt[6])[1] = 0;

    /* Prepare GDT pointer structure */
    GDTPtr gdt_ptr = {.limit = sizeof(GDTEntry) * GDT_ENTRIES - 1, .base = (uint64_t)gdt};

    /* Load GDT and update segment registers */
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

    /* Load Task State Segment register */
    __asm__ volatile("ltr %%ax" : : "a"(0x28));
}