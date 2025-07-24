/**
 * @file idt.c
 * @brief Kernel Interrupt Manager Implementation
 *
 * Sets up the Interrupt Descriptor Table (IDT), handles exceptions and hardware interrupts.
 */

#include "efibind.h"
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
#include <stdint.h>

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
    void** virtual_isr = EXTERN(void*, isr_stub_table);
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
    case 0x0: /* Divide Error (#DE) */
        process_signal(*CURRENT_PROCESS, SIGFPE);
        break;
    case 0x1: /* Debug (#DB) */
        process_signal(*CURRENT_PROCESS, SIGTRAP);
        break;
    case 0x2: /* Non-Maskable Interrupt (NMI) */
        break;
    case 0x3: /* Breakpoint (#BP) */
        process_signal(*CURRENT_PROCESS, SIGTRAP);
        break;
    case 0x4: /* Overflow (#OF) */
        process_signal(*CURRENT_PROCESS, SIGSEGV);
        break;
    case 0x5: /* Bound Range Exceeded (#BR) */
        process_signal(*CURRENT_PROCESS, SIGSEGV);
        break;
    case 0x6: /* Invalid Opcode (#UD) */
        process_signal(*CURRENT_PROCESS, SIGILL);
        break;
    case 0x7: /* Device Not Available (#NM) */
        process_signal(*CURRENT_PROCESS, SIGSEGV);
        break;
    case 0x8: /* Double Fault (#DF) */
        /* Kills the system (unrecoverable)  */
        break;
    case 0x9: /* Coprocessor Segment Overrun */
        /* Legacy, no longer used */
        break;
    case 0xA: /* Invalid TSS (#TS) */
        process_signal(*CURRENT_PROCESS, SIGBUS);
        break;
    case 0xB: /* Segment Not Present (#NP) */
        process_signal(*CURRENT_PROCESS, SIGSEGV);
        break;
    case 0xC: /* Stack-Segment Fault (#SS) */
        process_signal(*CURRENT_PROCESS, SIGSEGV);
        break;
    case 0xD: /* General Protection Fault (#GP) */
    {
        uint16_t selector = ((INTERRUPT_INFO->error_code) & 0xFFFF);
        bool is_external = (((INTERRUPT_INFO->error_code) >> 17) & 1);
        bool is_ldt_or_idt = (((INTERRUPT_INFO->error_code) >> 16) & 1);
        if (INTERRUPT_INFO->error_code == 0)
        {
            process_signal(*CURRENT_PROCESS, SIGILL);
        }
        else if (is_external || is_ldt_or_idt || !selector)
        {
            process_signal(*CURRENT_PROCESS, SIGSEGV);
        }
        else
        {
            process_signal(*CURRENT_PROCESS, SIGILL);
        }
    }
    break;
    case 0xE: /* Page Fault (#PF) */
    {
        uint64_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2) : :);

        uint64_t current_cr3;
        __asm__ volatile("mov %%cr3, %0\n\t" : "=r"(current_cr3) : :);
        __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(*KERNEL_PAGE_TABLE) :);

        if (INTERRUPT_INFO->error_code & 2) /* Write Fault */
        {
            page_lookup_result_t entry_results = pageTable_find_entry(&(*CURRENT_PROCESS)->page_table, cr2);
            if (entry_results.size && entry_results.entry & PAGE_COW)
            {
                uint64_t page = (uint64_t)pages_allocatePage(entry_results.size);
                kmemcpy((void*)page, (void*)(entry_results.entry & PAGE_MASK), entry_results.size);
                pageTable_addPage(&(*CURRENT_PROCESS)->page_table, (void*)cr2, page / entry_results.size, 1, entry_results.size, 4);
                __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(current_cr3) :);
                return;
            }
        }

        bool present = (INTERRUPT_INFO->error_code & 0x1) != 0;

        if (!present)
        {
            process_signal(*CURRENT_PROCESS, SIGSEGV);
        }
        else
        {
            process_signal(*CURRENT_PROCESS, SIGBUS);
        }
    }
    break;
    case 0xF: /* Reserved */
        /* Not used */
        break;
    case 0x10: /* x87 Floating-Point Exception (#MF) */
        process_signal(*CURRENT_PROCESS, SIGFPE);
        break;
    case 0x11: /* Alignment Check (#AC) */
        process_signal(*CURRENT_PROCESS, SIGBUS);
        break;
    case 0x12: /* Machine Check (#MC) */
        break;
    case 0x13: /* SIMD Floating-Point Exception (#XM) */
        process_signal(*CURRENT_PROCESS, SIGFPE);
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
            INTERRUPT_INFO->cr3 = (uint64_t)(*CURRENT_PROCESS)->page_table;
            INTERRUPT_INFO->rsp = (uint64_t)(&(*CURRENT_PROCESS)->process_stack_signature);
            TSS->ist1 = (uint64_t)(*CURRENT_PROCESS) + sizeof(process_stack_layout_t);

   
         
        }
        break;
        default:
        // will handle later
        __asm__ volatile("mov %0, %%r12\n\t" : : "r"(INTERRUPT_INFO->rsp));
        __asm__ volatile("mov %0, %%r15\n\t" : : "r"(INTERRUPT_INFO->irq_number));
            __asm__ volatile("hlt\n");

            break;
        }
    }
}

void check_signal()
{
    if ((*CURRENT_PROCESS)->signal == SIGNONE)
        return;

    switch ((*CURRENT_PROCESS)->signal)
    {
    case SIGQUIT:
    case SIGILL:
    case SIGABRT:
    case SIGFPE:
    case SIGSEGV:
    case SIGBUS:
    case SIGXCPU:
    case SIGTRAP:
    case SIGXFSZ:
    case SIGSYS:
        /* Terminate and Core Dump */
        process_exit((*CURRENT_PROCESS), (*CURRENT_PROCESS)->signal | (1 << 7));
        break;
    case SIGTERM:
    case SIGHUP:
    case SIGINT:
    case SIGPIPE:
    case SIGSTKFLT:
    case SIGVTALRM:
    case SIGALRM:
    case SIGUSR1:
    case SIGPROF:
    case SIGPWR:
    case SIGUSR2:
    case SIGIO:
        /* Terminate */
        process_exit((*CURRENT_PROCESS), (*CURRENT_PROCESS)->signal);
        break;
    case SIGCHLD:
    case SIGURG:
    case SIGWINCH:
        /* Ignore */
        break;
    default:
        break;
    }

    (*CURRENT_PROCESS)->signal = SIGNONE;
}