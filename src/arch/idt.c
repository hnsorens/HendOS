/**
 * @file idt.c
 * @brief Kernel Interrupt Manager Implementation
 *
 * Complete implementation of interrupt and exception handling
 * with all cases explicitly documented.
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
#include <memory/pmm.h>
#include <misc/debug.h>

#define IDT_MAX_DESCRIPTORS 257     ///< Total number of IDT entries
#define GDT_OFFSET_KERNEL_CODE 0x08 ///< Kernel code segment selector
#define GDT_OFFSET_KERNEL_DATA 0x10 ///< Kernel data segment selector

/* Interrupt vector definitions */
#define IRQ1 0x21  ///< Keyboard interrupt vector
#define IRQ12 0x2C ///< Mouse interrupt vector

extern void* isr_stub_table[]; ///< Array of ISR stub addresses
idt_data_t data;               ///< Global IDT data structure

/* ==================== IDT Descriptor Setup ==================== */

/**
 * @brief Configures an IDT descriptor
 * @param vector Interrupt vector number (0-255)
 * @param isr Pointer to interrupt handler function
 * @param dpl Descriptor privilege level (0-3)
 * @param ist Interrupt stack table index (0-7)
 */
void idt_set_descriptor(uint8_t vector, void* isr, uint8_t dpl, uint8_t ist)
{
    idt_entry_t* descriptor = &data.idt[vector];

    /* Configure descriptor flags */
    uint8_t flags = (1 << 7) |           // Present bit
                    ((dpl & 0x3) << 5) | // Descriptor Privilege Level
                    (0 << 4) |           // Storage Segment = 0
                    0xE;                 // Type = 0xE (interrupt gate)

    descriptor->isr_low = (uint64_t)isr & 0xFFFF;
    descriptor->kernel_cs = GDT_OFFSET_KERNEL_CODE;
    descriptor->ist = ist;
    descriptor->attributes = flags;
    descriptor->isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->isr_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;
}

/* ==================== PIC Initialization ==================== */

/**
 * @brief Initializes the Programmable Interrupt Controller
 *
 * Configures PIC offsets, cascade wiring, and operating mode.
 * Masks all interrupts initially.
 */
void pic_init()
{
    /* Mask all interrupts */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    /* Initialize PICs */
    outb(PIC1_CMD, 0x11); /* ICW1: Initialize, edge-triggered */
    outb(PIC2_CMD, 0x11);

    /* Set vector offsets */
    outb(PIC1_DATA, 0x20); /* IRQ 0-7 -> INT 0x20-0x27 */
    outb(PIC2_DATA, 0x28); /* IRQ 8-15 -> INT 0x28-0x2F */

    /* Configure master/slave wiring */
    outb(PIC1_DATA, 0x04); /* Slave PIC at IRQ2 */
    outb(PIC2_DATA, 0x02); /* Cascade identity */

    /* Set 8086 mode */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
}

/* ==================== Public API Implementation ==================== */

/**
 * @brief Initializes the Interrupt Descriptor Table
 *
 * Sets up all 256 IDT entries, configures the PIC,
 * and loads the IDT register.
 */
void idt_init()
{
    *TEMP = 0; /* Temporary debugging variable */
    pic_init();

    /* Setup IDT pointer */
    data.IDTr_t.base = (uintptr_t)&data.idt[0];
    data.IDTr_t.limit = (uint16_t)sizeof(idt_entry_t) * (IDT_MAX_DESCRIPTORS - 1);

    /* Initialize all IDT entries */
    void** virtual_isr = EXTERN(void**, isr_stub_table);
    for (int vector = 0; vector < 256; vector++)
    {
        idt_set_descriptor(vector, EXTERN(void*, virtual_isr[vector]), 0, 1);
    }

    /* Special handlers */
    idt_set_descriptor(0x20, EXTERN(void*, virtual_isr[0x20]), 0, 1); /* Timer */
    idt_set_descriptor(0x80, EXTERN(void*, virtual_isr[0x80]), 3, 1); /* Syscall */

    /* Load IDT */
    __asm__ volatile("lidt %0" : : "m"(data.IDTr_t));
}

/**
 * @brief Handles CPU exceptions
 * @noreturn
 *
 * Dispatches exceptions to appropriate signal handlers
 * or performs system shutdown for fatal errors.
 */
void idt_exception_handler()
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
        /* No action, hardware-specific */
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
        /* Fatal error - system cannot recover */
        __asm__ volatile("hlt");
        break;

    case 0x9: /* Coprocessor Segment Overrun */
        /* Legacy exception - no action */
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
        break;
    }

    case 0xE: /* Page Fault (#PF) */
    {
        uint64_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2) : :);

        uint64_t current_cr3;
        __asm__ volatile("mov %%cr3, %0\n\t" : "=r"(current_cr3) : :);
        __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(*KERNEL_PAGE_TABLE) :);

        if (INTERRUPT_INFO->error_code & 2) /* Write Fault */
        {
            page_lookup_result_t entry_results = vmm_find_entry(&(*CURRENT_PROCESS)->page_table, cr2);
            if (entry_results.size && entry_results.entry & PAGE_COW)
            {
                uint64_t page = pmm_allocate(entry_results.size);
                kmemcpy(page, entry_results.entry & PAGE_MASK, entry_results.size);
                vmm_add_page(&(*CURRENT_PROCESS)->page_table, cr2, page / entry_results.size, 1, entry_results.size, 4);
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
        break;
    }

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
        /* Fatal hardware error */
        __asm__ volatile("hlt");
        break;

    case 0x13: /* SIMD Floating-Point Exception (#XM) */
        process_signal(*CURRENT_PROCESS, SIGFPE);
        break;

    default:
        /* Restore all registers and halt for unknown exceptions */
        __asm__ volatile("mov %0, %%rsp\n\t" : : "r"(INTERRUPT_INFO->rsp) :);
        __asm__ volatile("pop %r15\n\t");
        __asm__ volatile("pop %r14\n\t");
        __asm__ volatile("pop %r13\n\t");
        __asm__ volatile("pop %r12\n\t");
        __asm__ volatile("pop %r11\n\t");
        __asm__ volatile("pop %r10\n\t");
        __asm__ volatile("pop %r9\n\t");
        __asm__ volatile("pop %r8\n\t");
        __asm__ volatile("pop %rbp\n\t");
        __asm__ volatile("pop %rdi\n\t");
        __asm__ volatile("pop %rsi\n\t");
        __asm__ volatile("pop %rdx\n\t");
        __asm__ volatile("pop %rcx\n\t");
        __asm__ volatile("pop %rbx\n\t");
        __asm__ volatile("pop %rax\n\t");
        __asm__ volatile("hlt\n\t");
    }
}

/**
 * @brief Handles hardware interrupts
 * @noreturn
 *
 * Processes timer ticks, keyboard input, mouse events,
 * and schedules context switches.
 */
void idt_interrupt_handler()
{
    switch (INTERRUPT_INFO->irq_number)
    {
    case 0x2C: /* Mouse interrupt */
        mouse_isr();
        break;

    case 0x21: /* Keyboard interrupt */
        keyboard_isr();
        vcon_handle_user_input();
        break;

    case 0x20: /* Timer interrupt */
    {
        /* Schedule next process */
        process_t* next = scheduler_nextProcess();
        (*CURRENT_PROCESS) = next;
        INTERRUPT_INFO->cr3 = (*CURRENT_PROCESS)->page_table;
        INTERRUPT_INFO->rsp = &(*CURRENT_PROCESS)->process_stack_signature;
        TSS->ist1 = (uint64_t)(*CURRENT_PROCESS) + sizeof(process_stack_layout_t);
        break;
    }

    default:
        /* Log unknown interrupt and halt */
        __asm__ volatile("mov %0, %%r12\n\t" : : "r"(INTERRUPT_INFO->irq_number));
        __asm__ volatile("hlt\n");
    }
}

/**
 * @brief Processes pending signals for current process
 *
 * Checks for pending signals and takes appropriate action
 * (termination, core dump, or ignoring) based on signal type.
 */
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