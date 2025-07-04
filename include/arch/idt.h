/**
 * @file idt.h
 * @brief Interrupt Descriptor Table Interface
 *
 * Provides structures and functions for managing x86-64 interrupts
 * and exception handling. The IDT is used to define interrupt
 * service routines for CPU exceptions and hardware interrupts.
 */

#ifndef IDT_H
#define IDT_H

#include <kint.h>
#include <memory/kmemory.h>

/**
 * @struct idt_entry_t
 * @brief Single entry in the Interrupt Descriptor Table
 *
 * Describes an interrupt or exception handler with its address,
 * segment selector, and attributes.
 */
typedef struct idt_entry_t
{
    uint16_t isr_low;   ///< Lower 16 bits of the ISR's address
    uint16_t kernel_cs; ///< GDT segment selector loaded into CS
    uint8_t ist;        ///< IST in TSS that CPU loads into RSP
    uint8_t attributes; ///< Type and attributes (see Intel SDM)
    uint16_t isr_mid;   ///< Middle 16 bits of the ISR's address
    uint32_t isr_high;  ///< Upper 32 bits of the ISR's address
    uint32_t reserved;  ///< Reserved (must be zero)
} __attribute__((packed)) idt_entry_t;

/**
 * @struct IDTr_t
 * @brief Pointer structure for LIDT instruction
 *
 * Contains the base address and size limit of the IDT.
 */
typedef struct IDTr_t
{
    uint16_t limit; ///< Size of IDT minus 1
    uint64_t base;  ///< Linear address of IDT
} __attribute__((packed)) IDTr_t;

/**
 * @struct idt_data_t
 * @brief Complete IDT structure
 *
 * Contains all IDT entries and the pointer structure.
 */
typedef struct idt_data_t
{
    idt_entry_t idt[256]; ///< Array of 256 interrupt descriptors
    IDTr_t IDTr_t;        ///< IDT pointer for LIDT instruction
} idt_data_t;

/**
 * @struct interrupt_info_t
 * @brief Interrupt context information
 *
 * Passed to interrupt handlers containing register state
 * and error information.
 */
typedef struct interrupt_info_t
{
    uint64_t irq_number; ///< Interrupt vector number
    uint64_t error_code; ///< CPU error code (if applicable)
    uint64_t cr3;        ///< Current page table base
    uint64_t rsp;        ///< Stack pointer at interrupt time
} __attribute__((packed)) interrupt_info_t;

#define INTERRUPT_INFO ((interrupt_info_t*)INTERRUPT_INFO_START) ///< Current interrupt context

/**
 * @brief Initializes the Interrupt Descriptor Table
 *
 * Configures all IDT entries and loads the IDT register.
 */
void idt_init();

/**
 * @brief Handles CPU exceptions
 */
void idt_exception_handler();

/**
 * @brief Handles hardware interrupts
 */
void idt_interrupt_handler();

#endif /* IDT_H */