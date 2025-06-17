/**
 * @file kmemory.h
 * @brief Kernel Memory Management Interface
 * 
 * Provides core memory management functions including allocation, deallocation, 
 * and memory operations for the kernel
 */

#ifndef K_GLOBALS_H
#define K_GLOBALS_H

#include <memory/kmemory.h>
#include <drivers/keyboard.h>
#include <drivers/fbcon.h>
#include <drivers/graphics.h>
#include <boot/bootServices.h>
#include <drivers/mouse.h>
#include <memory/pageTable.h>
#include <kernel/process.h>
#include <kernel/device.h>
#include <arch/gdt.h>
#include <drivers/vcon.h>
#include <fs/fontLoader.h>
#include <kernel/pidHashTable.h>

/* Constants */
#define GLOBAL_VARS_END 0x37B9E00000 ///< End address for global variables allocation

/* Utility Macros*/
#define OFFSET(global) (uint8_t*)global

/**
 * @brief Calculate address for a single variable
 * @param type Data type of the global
 * @param last_global Previous global variable in the allocation chain
 */
#define __global_1___(type, last_global) ((type*)(OFFSET(last_global) - sizeof(type)))
#define createGlobal(type, last_global) __global_1___(type, last_global)

/**
 * @brief Calculate address for an array of global variables
 * @param type Data type of array element
 * @param count Number of elements in array
 * @param last_global Previous global variable in the allocation chain
 */
#define __global_array_1___(type, count, last_global) ((type*)(OFFSET(last_global) - sizeof(type)*count))
#define createGlobalArray(type, count, last_global) __global_array_1___(type, count, last_global)

/**
 * Global Variables Declaration
 * 
 * Variables are declared in reverse allocation order (last to first)
 * Each declaration includes a brief description of its purpose
 */

/* Memory Management */
#define HEAP_DATA ((heap_data_t*)(GLOBAL_VARS_END - sizeof(heap_data_t)))
#define NUM_2MB_PAGES           createGlobal(uint64_t,                      HEAP_DATA               ) ///< Total number of 2 Mb pages in system memory
#define NUM_4KB_PAGES           createGlobal(uint64_t,                      NUM_2MB_PAGES           ) ///< Total number of 4 Kb pages in system memory
#define BITMAP_2MB              createGlobal(uint64_t*,                     NUM_4KB_PAGES           ) ///< 2 Mb page allocation bitmap
#define BITMAP_4KB              createGlobal(uint64_t*,                     BITMAP_2MB              ) ///< 4 Kb page allocation bitmap
#define FREE_STACK_2MB          createGlobal(uint32_t*,                     BITMAP_4KB              ) ///< 2 Mb page allocation free stack
#define FREE_STACK_4KB          createGlobal(uint32_t*,                     FREE_STACK_2MB          ) ///< 4 Kb page allocation free stack
#define FREE_STACK_2MB_TOP      createGlobal(uint32_t,                      FREE_STACK_4KB          ) ///< 2 Mb page allocation free stack top
#define FREE_STACK_4KB_TOP      createGlobal(uint32_t,                      FREE_STACK_2MB_TOP      ) ///< 4 Kb page allocation free stack top
#define KERNEL_PAGE_TABLE       createGlobal(page_table_t,                  FREE_STACK_4KB_TOP      ) ///< Kernels paging table
#define PROCESS_MEM_FREE_STACK  createGlobalArray(uint16_t,         2056,   KERNEL_PAGE_TABLE       ) ///< Free stack for kernel page table process entries
#define MEMORY_REGIONS          createGlobalArray(MemoryRegion,     10,     PROCESS_MEM_FREE_STACK  ) ///< Preboot allocated memory regions for kernel
#define PREBOOT_INFO            createGlobal(preboot_info_t,                MEMORY_REGIONS          ) ///< Information gathered before exiting boot services

/* Process Management */
#define PID                     createGlobal(uint64_t,                      PREBOOT_INFO            ) ///< Process ID counter
#define CURRENT_PROCESS         createGlobal(process_t*,                    PID                     ) ///< Pointer to the current process context
#define PROCESSES               createGlobal(process_t*,                    CURRENT_PROCESS         ) ///< Process Context Loop
#define PROCESS_COUNT           createGlobal(uint64_t,                      PROCESSES               ) ///< Number of processes running
#define TSS                     createGlobal(TSS64,                         PROCESS_COUNT           ) ///< TSS which contains the stack that is switched to when interrupts happen
#define PID_MAP                 createGlobal(pid_hash_table_t,              TSS                     ) ///< Hash table for process lookup with PID
#define PGID_MAP                createGlobal(pid_hash_table_t,              PID_MAP                 ) ///< Hash table for process group lookup
#define SID_MAP                 createGlobal(pid_hash_table_t,              PGID_MAP                ) ///< Hash table for session lookup

/* Graphics System */
#define FBCON                   createGlobal(fbcon_t,                       SID_MAP                 ) ///< The frambuffer console object
#define INTEGRATED_FONT         createGlobal(Font,                          FBCON                   ) ///< Font object for FBCON
#define GRAPHICS_LAYERS         createGlobalArray(Layer*,           128,    INTEGRATED_FONT         ) ///< Graphics layers for rendering
#define GRAPHICS_CONTEXT        createGlobal(GraphicsContext,               GRAPHICS_LAYERS         ) ///< Contains all graphics information
#define GRAPHICS_LAYER_COUNT    createGlobal(uint32_t,                      GRAPHICS_CONTEXT        ) ///< Number of active graphics layers

/* Filesystem */
#define FILESYSTEM              createGlobal(ext2_fs_t,                     GRAPHICS_LAYER_COUNT    ) ///< Filesystem objecy
#define ROOT                    createGlobal(directory_t,                   FILESYSTEM              ) ///< Root directory
#define DEV                     createGlobal(directory_t*,                  ROOT                    ) ///< Dev directory

/* Hardware State */
#define KEYBOARD_STATE          createGlobal(keyboard_state_t,              DEV                     ) ///< Current keyboard state from interrupts
#define MOUSE_STATE             createGlobal(mouse_t,                       KEYBOARD_STATE          ) ///< Current mouse state from interrupts

/* Device Layers*/
#define DEVICE_MANAGER          createGlobal(dev_manager_t,                 MOUSE_STATE             ) ///< Global Device Manager
#define VCONS                   createGlobalArray(vcon_t,    VCON_COUNT,    DEVICE_MANAGER          ) ///< Global Array of All Virtual Console Drivers

/* Syscalls */
typedef void(*syscall_fn)();
#define SYSCALLS                createGlobalArray(syscall_fn,       512,    VCONS                   ) ///< List of all syscall function pointers

#define TEMP        createGlobal(uint64_t, SYSCALLS)

#endif /* K_GLOBALS_H */