/**
 * @file idt.h
 * @brief Interrupt Descriptor Table (IDT) Interface
 *
 * Declares structures and functions for setting up the x86_64 IDT and handling exceptions/interrupts.
 */
#ifndef IDT_H
#define IDT_H

#include <kint.h>
#include <memory/kmemory.h>

typedef struct
{
    uint16_t isr_low;   // The lower 16 bits of the ISR's address
    uint16_t kernel_cs; // The GDT segment selector that the CPU will load into
                        // CS before calling the ISR
    uint8_t ist;        // The IST in the TSS that the CPU will load into RSP; set to
                        // zero for now
    uint8_t attributes; // Type and attributes; see the IDT page
    uint16_t isr_mid;   // The higher 16 bits of the lower 32 bits of the ISR's address
    uint32_t isr_high;  // The higher 32 bits of the ISR's address
    uint32_t reserved;  // Set to zero
} __attribute__((packed)) IDTEntry;

typedef struct
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) IDTr;

typedef struct
{
    IDTEntry idt[256];
    IDTr idtr;
} IDTData;

typedef struct interrupt_info_t
{
    uint64_t irq_number;
    uint64_t error_code;
    uint64_t cr3;
    uint64_t rsp;
} __attribute__((packed)) interrupt_info_t;

#define INTERRUPT_INFO ((interrupt_info_t*)INTERRUPT_INFO_START)

void KERNEL_InitIDT();

__attribute__((noreturn)) void exception_handler();
__attribute__((noreturn)) void interrupt_handler();

#endif /* IDT_H */