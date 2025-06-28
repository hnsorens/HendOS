/* memoryMap.h */
#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#define MAX_ADDRESS 0xFFFFFFFFFFFF
#define ADDRESS_SECTION_SIZE 0x2000000000

/**
 * This file contains all virtual memory map locations for the kernel and processes
 */

/* physical memory space*/

/* ------------------------ first 128 gb ------------------------------ */

/* kernel space */

/* ------------------------ second 128 gb ------------------------------ */

#define KERNEL_CODE_START 0x3800000000 // 0x00000FFF00000000

#define KERNEL_STACK_START 0x37FF000000 // 0x00000FFEFF000000

#define INTERRUPT_STACK_START 0x37FFFFFF00
#define INTERRUPT_INFO_START 0x37FFFFFEE0

#define KERNEL_STACK_SIZE 0x1000000

#define KERNEL_HEAP_START 0x37BF000000 // 0x00000FFEBF000000
#define KERNEL_HEAP_SIZE 0x40000000

#define PAGE_ALLOCATION_TABLE_START 0x37BE000000 // 0x00000FFEBE000000
#define PAGE_ALLOCATION_TABLE_SIZE 0x1000000

#define PAGE_TABLE_START 0x37BA000000 // 0x00000FFEBA800000
#define PAGE_TABLE_SIZE 0x1000

#define TRAMPOLINE_START 0x37B9E00000 // 0x00000FFEBA600000
#define TRAMPOLINE_SIZE 0x200000

#define GLOBAL_VARS_START 0x37B9C00000 // 0x00000FFEBA400000
#define GLOBAL_VARS_SIZE 0x200000

#define FRAMEBUFFER_START 0x2000000000
#define FRAMEBUFFER_SIZE 0x10000000

/* Process code starts at the third section. from now on each process has 128 gb sections which
 * allows 2045 concurrent running processes at once */

#define EXTERN(type, var) ((type*)((char*)(var) + KERNEL_CODE_START))

#endif