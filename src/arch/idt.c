/**
 * @file idt.c
 * @brief Kernel Interrupt Manager Implementation
 *
 *
 */

#include <arch/idt.h>
#include <arch/pic.h>
#include <drivers/fbcon.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <efi.h>
#include <efilib.h>
#include <kernel/scheduler.h>
#include <kernel/syscalls.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <memory/memoryMap.h>
#include <memory/paging.h>
#include <misc/debug.h>

#define IDT_MAX_DESCRIPTORS 257
#define GDT_OFFSET_KERNEL_CODE 0x08 /* Selector for gdt[1] (kernel code) */
#define GDT_OFFSET_KERNEL_DATA 0x10 /* Selector for gdt[2] (kernel data) */

/* Define interrupt vectors */
#define IRQ1 0x21  /* Keyboard interrupt vector */
#define IRQ12 0x2C /* Mouse interrupt vector */

/* Allowed */
extern void* isr_stub_table[];
IDTData data;

/* ================================= Private API ======================================*/

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t dpl, uint8_t ist)
{
    IDTEntry* descriptor = &data.idt[vector];

    uint8_t flags = (1 << 7)             // Present
                    | ((dpl & 0x3) << 5) // DPL
                    | (0 << 4)           // Storage Segment = 0
                    | 0xE;               // Type = 0xE (interrupt gate)

    descriptor->isr_low = (uint64_t)isr & 0xFFFF;
    descriptor->kernel_cs = GDT_OFFSET_KERNEL_CODE;
    descriptor->ist = ist;
    descriptor->attributes = flags;
    descriptor->isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->isr_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;
}

void pic_init()
{
    /* Mask all IRQs */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    /* Initialize the PIC */
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);
    outb(PIC1_DATA, 0x20); /* PIC1 offset (IRQ 0-7 mapped to IDT 32-39)   */
    outb(PIC2_DATA, 0x28); /* PIC2 offset (IRQ 8-15 mapped to IDT 40-47)  */
    outb(PIC1_DATA, 0x04); /* Tell PIC1 about PIC2 at IRQ2                */
    outb(PIC2_DATA, 0x02); /* Tell PIC2 its cascade identity (0000 0010)  */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
}

/* ========================================= Public API ===========================================
 */

void KERNEL_InitIDT()
{
    *TEMP = 0;
    pic_init();

    data.idtr.base = (uintptr_t)&data.idt[0];
    data.idtr.limit = (uint16_t)sizeof(IDTEntry) * (IDT_MAX_DESCRIPTORS - 1);
    void** virtual_isr = EXTERN(void**, isr_stub_table);
    for (int vector = 0; vector < 256; vector++)
    {
        idt_set_descriptor(vector, EXTERN(void*, virtual_isr[vector]), 0, 1);
    }
    idt_set_descriptor(0x20, EXTERN(void*, virtual_isr[0x20]), 0, 1);
    idt_set_descriptor(0x80, EXTERN(void*, virtual_isr[0x80]), 3, 1);
    __asm__ volatile("lidt %0" : : "m"(data.idtr)); /* load the new IDT */
}

void exception_handler()
{
    switch (INTERRUPT_INFO->irq_number)
    {
    case 0xE:
    {
        uint64_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2) : :);

        uint64_t current_cr3;
        __asm__ volatile("mov %%cr3, %0\n\t" : "=r"(current_cr3) : :);
        __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(KERNEL_PAGE_TABLE->pml4) :);

        if (INTERRUPT_INFO->error_code & 2) /* Write Fault */
        {
            page_lookup_result_t entry_results = pageTable_find_entry((*CURRENT_PROCESS)->page_table, cr2);
            if (entry_results.size && entry_results.entry & PAGE_COW)
            {
                uint64_t page = pages_allocatePage(entry_results.size);
                kmemcpy(page, entry_results.entry & PAGE_MASK, entry_results.size);
                pageTable_addPage((*CURRENT_PROCESS)->page_table, cr2, page / entry_results.size, 1, entry_results.size, 4);
                __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(current_cr3) :);
                return;
            }
            else
            {
                __asm__ volatile("hlt\n\t");
            }
        }
        __asm__ volatile("hlt\n\t");
    }
    break;
    default:
        __asm__ volatile("mov %0, %%rsp\n\t" : : "r"(INTERRUPT_INFO->rsp) :);
        __asm__ volatile("pop %%r15\n\t"
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
        __asm__ volatile("pop %%r15\n\t"
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
        __asm__ volatile("mov %0, %%rsp\n\t" : : "r"(INTERRUPT_INFO) :);
        __asm__ volatile("pop %%rdx\n\t"
                         "pop %%rcx\n\t"
                         "pop %%rbx\n\t"
                         "pop %%rax\n\t"
                         :
                         :
                         :);
        __asm__ volatile("hlt\n\t");
    }
}

void interrupt_handler()
{
    {
        switch (INTERRUPT_INFO->irq_number)
        {
        case 0x2C:
            mouse_isr();
            break;
        case 0x21:
            keyboard_isr();
            vcon_handle_user_input();
            break;
        case 0x20:
        {

            process_t* next = scheduler_nextProcess();

            (*CURRENT_PROCESS) = next;
            INTERRUPT_INFO->cr3 = (*CURRENT_PROCESS)->page_table->pml4;
            INTERRUPT_INFO->rsp = &(*CURRENT_PROCESS)->process_stack_signature;
            TSS->ist1 = (uint64_t)(*CURRENT_PROCESS) + sizeof(process_stack_layout_t);
        }
        break;
        default:
            // will handle later
            __asm__ volatile("mov %0, %%r12\n\t" : : "r"(INTERRUPT_INFO->irq_number));
            __asm__ volatile("mov %0, %%r15\n\t" : : "r"(INTERRUPT_INFO->irq_number));
            __asm__ volatile("hlt\n");

            break;
        }
    }
}