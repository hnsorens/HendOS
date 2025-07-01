/**
 * @file kmemory.h
 * @brief Kernel Memory Management Interface
 *
 * Provides core memory management functions including allocation, deallocation,
 * and memory operations for the kernel
 */

#ifndef K_GLOBALS_H
#define K_GLOBALS_H

#include <arch/gdt.h>
#include <arch/idt.h>
#include <boot/bootServices.h>
#include <drivers/fbcon.h>
#include <drivers/graphics.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vcon.h>
#include <fs/fdm.h>
#include <fs/fontLoader.h>
#include <fs/vfs.h>
#include <kernel/device.h>
#include <kernel/pidHashTable.h>
#include <kernel/process.h>
#include <memory/kmemory.h>
#include <memory/kpool.h>
#include <memory/memoryMap.h>
#include <memory/pageTable.h>

/* Constants */
#define GLOBAL_VARS_END 0xFFFF860000200000 ///< End address for global variables allocation

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
#define __global_array_1___(type, count, last_global) ((type*)(OFFSET(last_global) - sizeof(type) * count))
#define createGlobalArray(type, count, last_global) __global_array_1___(type, count, last_global)

/**
 * Global Variables Declaration
 *
 * Variables are declared in reverse allocation order (last to first)
 * Each declaration includes a brief description of its purpose
 */

/* Memory Management */
#define HEAP_DATA ((heap_data_t*)(GLOBAL_VARS_END - sizeof(heap_data_t)))
#define NUM_2MB_PAGES createGlobal(uint64_t, HEAP_DATA)                       ///< Total number of 2 Mb pages in system memory
#define NUM_4KB_PAGES createGlobal(uint64_t, NUM_2MB_PAGES)                   ///< Total number of 4 Kb pages in system memory
#define BITMAP_2MB createGlobal(uint64_t*, NUM_4KB_PAGES)                     ///< 2 Mb page allocation bitmap
#define BITMAP_4KB createGlobal(uint64_t*, BITMAP_2MB)                        ///< 4 Kb page allocation bitmap
#define FREE_STACK_2MB createGlobal(uint32_t*, BITMAP_4KB)                    ///< 2 Mb page allocation free stack
#define FREE_STACK_4KB createGlobal(uint32_t*, FREE_STACK_2MB)                ///< 4 Kb page allocation free stack
#define FREE_STACK_2MB_TOP createGlobal(uint32_t, FREE_STACK_4KB)             ///< 2 Mb page allocation free stack top
#define FREE_STACK_4KB_TOP createGlobal(uint32_t, FREE_STACK_2MB_TOP)         ///< 4 Kb page allocation free stack top
#define KERNEL_PAGE_TABLE createGlobal(page_table_t, FREE_STACK_4KB_TOP)      ///< Kernels paging table
#define MEMORY_REGIONS createGlobalArray(MemoryRegion, 10, KERNEL_PAGE_TABLE) ///< Preboot allocated memory regions for kernel
#define PREBOOT_INFO createGlobal(preboot_info_t, MEMORY_REGIONS)             ///< Information gathered before exiting boot services
#define TEMP_MEMORY createGlobal(uint64_t*, PREBOOT_INFO)                     ///< Temporary memory for small allocations (2mb)

/* Memory Pools */
#define MEMORY_POOL_COUNTER createGlobal(uint64_t, TEMP_MEMORY)                ///< Counter for allocating pools
#define PROCESS_POOL createGlobal(kernel_memory_pool_t*, MEMORY_POOL_COUNTER)  ///< Memory pool for Process headers
#define INODE_POOL createGlobal(kernel_memory_pool_t*, PROCESS_POOL)           ///< Memory pool for Inodes
#define VFS_ENTRY_POOL createGlobal(kernel_memory_pool_t*, INODE_POOL)         ///< Memory pool for VFS Entries
#define OPEN_FILE_POOL createGlobal(kernel_memory_pool_t*, VFS_ENTRY_POOL)     ///< Memory pool for Open Files
#define PROCESS_GROUP_POOL createGlobal(kernel_memory_pool_t*, OPEN_FILE_POOL) ///< Memory pool for Process Groups
#define SESSION_POOL createGlobal(kernel_memory_pool_t*, PROCESS_GROUP_POOL)   ///< Memory pool for Sessions
#define FD_ENTRY_POOL createGlobal(kernel_memory_pool_t*, SESSION_POOL)        ///< Memory pool for file descriptor entries

/* Process Management */
#define PID createGlobal(uint64_t, FD_ENTRY_POOL)           ///< Process ID counter
#define CURRENT_PROCESS createGlobal(process_t*, PID)       ///< Pointer to the current process context
#define PROCESSES createGlobal(process_t*, CURRENT_PROCESS) ///< Process Context Loop
#define PROCESS_COUNT createGlobal(uint64_t, PROCESSES)     ///< Number of processes running
#define TSS createGlobal(TSS64, PROCESS_COUNT)              ///< TSS which contains the stack that is switched to when interrupts happen
#define PID_MAP createGlobal(pid_hash_table_t, TSS)         ///< Hash table for process lookup with PID
#define PGID_MAP createGlobal(pid_hash_table_t, PID_MAP)    ///< Hash table for process group lookup
#define SID_MAP createGlobal(pid_hash_table_t, PGID_MAP)    ///< Hash table for session lookup

/* Graphics System */
#define FBCON createGlobal(fbcon_t, SID_MAP)                            ///< The frambuffer console object
#define INTEGRATED_FONT createGlobal(font_t, FBCON)                     ///< Font object for FBCON
#define GRAPHICS_LAYERS createGlobalArray(Layer*, 128, INTEGRATED_FONT) ///< Graphics layers for rendering
#define GRAPHICS_CONTEXT createGlobal(GraphicsContext, GRAPHICS_LAYERS) ///< Contains all graphics information
#define GRAPHICS_LAYER_COUNT createGlobal(uint32_t, GRAPHICS_CONTEXT)   ///< Number of active graphics layers

/* Filesystem */
#define FILESYSTEM createGlobal(ext2_fs_t, GRAPHICS_LAYER_COUNT) ///< Filesystem objecy
#define ROOT createGlobal(vfs_entry_t, FILESYSTEM)               ///< Root directory
#define DEV createGlobal(vfs_entry_t*, ROOT)                     ///< Root directory
#define PATH createGlobalArray(char, 4096, DEV)                  ///< Temp Path that can be Used to build paths

/* Hardware State */
#define KEYBOARD_STATE createGlobal(keyboard_state_t, PATH) ///< Current keyboard state from interrupts
#define MOUSE_STATE createGlobal(mouse_t, KEYBOARD_STATE)   ///< Current mouse state from interrupts

/* Device Layers*/
#define VCONS createGlobalArray(vcon_t, VCON_COUNT, MOUSE_STATE) ///< Global Array of All Virtual Console Drivers

/* Syscalls */
typedef void (*syscall_fn)();
#define SYSCALLS createGlobalArray(syscall_fn, 512, VCONS) ///< List of all syscall function pointers

#define LAST_GLOBAL SYSCALLS

#define GLOBALS_SIZE (char*)(GLOBAL_VARS_END) - (char*)(LAST_GLOBAL)

#define TEMP createGlobal(uint64_t, SYSCALLS)

#endif /* K_GLOBALS_H */