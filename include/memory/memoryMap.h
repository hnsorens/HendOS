/* memoryMap.h */
#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#define MAX_ADDRESS 0xFFFFFFFFFFFF
#define ADDRESS_SECTION_SIZE 0x2000000000

/**
 * This file contains all virtual memory map locations for the kernel
 */

#define KERNEL_CODE_START 0x800000000000 /* 128tb */
#define KERNEL_CODE_SIZE 0x8000000000    /* 512gb */

#define PAGE_TABLE_START 0x808000000000 /* 128tb + 512gb */
#define PAGE_TABLE_SIZE 0x8000000000    /* 512gb */

#define KERNEL_STACK_START 0x810000000000 /* 129tb */
#define INTERRUPT_STACK_START 0x82FFFFFFFF00
#define INTERRUPT_INFO_START 0x82FFFFFFFEE0
#define KERNEL_STACK_SIZE 0x10000000000 /* 1tb */

#define PAGE_ALLOCATION_TABLE_START 0x820000000000 /* 130tb */
#define PAGE_ALLOCATION_TABLE_SIZE 0x10000000000   /* 1tb */

#define KERNEL_HEAP_START 0x830000000000 /* 131tb */
#define KERNEL_HEAP_SIZE 0x10000000000   /* 1tb */

#define GLOBAL_VARS_START 0x840000000000 /* 132tb */
#define GLOBAL_VARS_SIZE 0x10000000000   /* 1tb */

#define FRAMEBUFFER_START 0x850000000000 /* 133tb */
#define FRAMEBUFFER_SIZE 0x10000000000   /* 1tb */

#define POOLS_START 0x940000000000 /* 148tb */
#define POOLS_SIZE 0x6C0000000000  /* 108tb */

/* Process code starts at the third section. from now on each process has 128 gb sections which
 * allows 2045 concurrent running processes at once */

#define EXTERN(type, var) ((type*)((char*)(var) + KERNEL_CODE_START))

#endif