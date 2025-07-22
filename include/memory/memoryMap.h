/**
 * @file memoryMap.h
 * @brief Kernel Virtual Memory Map Definitions
 *
 * Defines all virtual memory map locations for the kernel and processes, including physical and kernel address space layout.
 */
#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#define MAX_ADDRESS 0xFFFFFFFFFFFF

/**
 * This file contains all virtual memory map locations for the kernel and processes
 */

/* physical memory space*/

/* ------------------------ first 128 gb ------------------------------ */

/* kernel space */

/* ------------------------ second 128 gb ------------------------------ */

#define KERNEL_CODE_START 0xFFFF800000000000 /* 128tb */

#define KERNEL_STACK_START 0xFFFF810000000000 /* 129tb */

#define INTERRUPT_STACK_START 0xFFFF810000FFFF00
#define INTERRUPT_INFO_START 0xFFFF810000FFFEE0

#define KERNEL_STACK_SIZE 0x1000000

#define KERNEL_HEAP_START 0xFFFF820000000000 /* 130tb */ // 0x00000FFEBF000000
#define KERNEL_HEAP_SIZE 0x40000000

#define PAGE_ALLOCATION_TABLE_START 0xFFFF830000000000 /* 131tb */ // 0x00000FFEBE000000
#define PAGE_ALLOCATION_TABLE_SIZE 0x1000000

#define PAGE_TABLE_START 0xFFFF840000000000 /* 132tb */ // 0x00000FFEBA800000
#define PAGE_TABLE_SIZE 0x1000

#define TRAMPOLINE_START 0xFFFF850000000000 /* 133tb */ // 0x00000FFEBA600000
#define TRAMPOLINE_SIZE 0x200000

#define GLOBAL_VARS_START 0xFFFF860000000000 /* 134tb */ /* 134tb */ // 0x00000FFEBA400000
#define GLOBAL_VARS_SIZE 0x200000

#define FRAMEBUFFER_START 0xFFFF870000000000 /* 135tb */
#define FRAMEBUFFER_SIZE 0x10000000

/* Process code starts at the third section. from now on each process has 128 gb sections which
 * allows 2045 concurrent running processes at once */

#define EXTERN(type, var) ((type*)((char*)(var) + KERNEL_CODE_START))

#endif