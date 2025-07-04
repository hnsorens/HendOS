/* gdt.h */
/**
 * @file gdt.h
 * @brief Global Descriptor Table Interface
 *
 * Provides structures and functions for managing the x86-64 Global Descriptor Table
 * and Task State Segment (TSS)
 */

#ifndef GDT_H
#define GDT_H

#define GDT_ENTRIES 7 ///< Number of entries in our GDT (including null, kernel/user segments, and TSS)

#include <kint.h>
#include <memory/kmemory.h>

/**
 * @struct TSS64
 * @brief x86-64 Task State Segment structure
 *
 * Contains processor state information for task switching and stack pointers
 * for different privilege levels
 */
typedef struct __attribute__((packed))
{
    uint32_t reserved0;   ///< Reserved field
    uint64_t rsp0;        ///< Ring 0 stack pointer
    uint64_t rsp1;        ///< Ring 1 stack pointer
    uint64_t rsp2;        ///< Ring 2 stack pointer
    uint64_t reserved1;   ///< Reserved field
    uint64_t ist1;        ///< Interrupt stack table entry 1
    uint64_t ist2;        ///< Interrupt stack table entry 2
    uint64_t ist3;        ///< Interrupt stack table entry 3
    uint64_t ist4;        ///< Interrupt stack table entry 4
    uint64_t ist5;        ///< Interrupt stack table entry 5
    uint64_t ist6;        ///< Interrupt stack table entry 6
    uint64_t ist7;        ///< Interrupt stack table entry 7
    uint64_t reserved2;   ///< Reserved field
    uint16_t reserved3;   ///< Reserved field
    uint16_t io_map_base; ///< I/O permission bitmap base address
} TSS64;

/**
 * @struct GDTEntry
 * @brief Single entry in the Global Descriptor Table
 *
 * Describes a memory segment or system segment (like TSS)
 */
typedef struct
{
    uint16_t limit_low;  ///< Lower 16 bits of segment limit
    uint16_t base_low;   ///< Lower 16 bits of segment base address
    uint8_t base_mid;    ///< Middle 8 bits of segment base address
    uint8_t access;      ///< Segment access flags and type
    uint8_t granularity; ///< Segment granularity and limit high bits
    uint8_t base_high;   ///< High 8 bits of segment base address
} __attribute__((packed)) GDTEntry;

/**
 * @struct GDTPtr
 * @brief Pointer structure passed to LGDT instruction
 *
 * Contains the base address and size limit of the GDT
 */
typedef struct
{
    uint16_t limit; ///< Size of GDT minus 1
    uint64_t base;  ///< Linear address of GDT
} __attribute__((packed)) GDTPtr;

/**
 * @brief Initializes the Global Descriptor Table and Task State Segment
 *
 * Sets up kernel and user code/data segments, initializes TSS,
 * and loads them into the processor
 */
void gdt_init();

#endif