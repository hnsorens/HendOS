/**
 * @file gdt.h
 * @brief Global Descriptor Table (GDT) Interface
 *
 * Declares structures and functions for setting up the x86_64 GDT and TSS.
 */
#ifndef GDT_H
#define GDT_H

#include <kint.h>
#include <memory/kmemory.h>

typedef struct __attribute__((packed))
{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
} TSS64;

typedef struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) GDTEntry;

typedef struct
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) GDTPtr;

void KERNEL_InitGDT();

#endif