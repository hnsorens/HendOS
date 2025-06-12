/**
 * @file syscalls.c
 * @brief System Call Implementation
 *
 * Contains the kernel-side implementation of system calls.
 * These functions execute in kernel mode with full privileges.
 */

#include <boot/elfLoader.h>
#include <kernel/syscalls.h>
#include <memory/kglobals.h>
#include <memory/memoryMap.h>
#include <misc/debug.h>

#define IA32_EFER 0xC0000080
#define IA32_STAR 0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_FMASK 0xC0000084

#define KERNEL_CS 0x08
#define USER_CS 0x1B

static uint64_t rdmsr(uint32_t msr)
{
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

/**
 * @brief Inline asm helpers for MSR read/write
 */
static inline void wrmsr(uint32_t msr, uint64_t value)
{
    uint32_t low = (uint32_t)(value & 0xFFFFFFFF);
    uint32_t high = (uint32_t)(value >> 32);
    __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high) : "memory");
}

/* Read CR4 register */
static inline uint64_t read_cr4()
{
    uint64_t val;
    __asm__ volatile("mov %%cr4, %0" : "=r"(val));
    return val;
}

/* Write CR4 register */
static inline void write_cr4(uint64_t val)
{
    __asm__ volatile("mov %0, %%cr4" : : "r"(val) : "memory");
}

/* ================================ Public API ============================= */

/**
 * @brief Initializes all syscalls
 */

extern void syscall_stub(void);

#define SYSCALL_WRITE 4
#define SYSCALL_EXIT 1
#define SYSCALL_EXECVE 2
#define SYSCALL_INPUT 3

void sys_do_nothing() {}
void syscall_init()
{
    // // Enable SYSCALL/SYSRET via EFER
    // uint64_t efer = rdmsr(IA32_EFER);
    // efer |= 1;
    // wrmsr(IA32_EFER, efer);

    // // Set STAR, LSTAR, FMASK as before
    // wrmsr(IA32_STAR, ((uint64_t)USER_CS << 48) | ((uint64_t)KERNEL_CS << 32));
    // wrmsr(IA32_LSTAR, (uint64_t)EXTERN(uint64_t, syscall_stub));
    // wrmsr(IA32_FMASK, (1 << 9)); // Mask IF (disable interrupts during syscall)

    /* Add all syscall functions */
    for (int i = 0; i < 512; i++)
    {
        SYSCALLS[i] = sys_do_nothing;
    }
    SYSCALLS[SYSCALL_WRITE] = sys_write;
    SYSCALLS[SYSCALL_INPUT] = sys_input;
    SYSCALLS[SYSCALL_EXIT] = sys_exit;
    SYSCALLS[SYSCALL_EXECVE] = execve;
}

/* ================================== SYSCALL API ===================================== */

#define SYSCALL_ARG1(name)                                                                         \
    uint64_t name;                                                                                 \
    __asm__ volatile("mov %%rdi, %0" : "=r"(name)::)
#define SYSCALL_ARG2(name)                                                                         \
    uint64_t name;                                                                                 \
    __asm__ volatile("mov %%rsi, %0" : "=r"(name)::)
#define SYSCALL_ARG3(name)                                                                         \
    uint64_t name;                                                                                 \
    __asm__ volatile("mov %%rdx, %0" : "=r"(name)::)
#define SYSCALL_ARG4(name)                                                                         \
    uint64_t name;                                                                                 \
    __asm__ volatile("mov %%r10, %0" : "=r"(name)::)
#define SYSCALL_ARG5(name)                                                                         \
    uint64_t name;                                                                                 \
    __asm__ volatile("mov %%r8, %0" : "=r"(name)::)
#define SYSCALL_ARG6(name)                                                                         \
    uint64_t name;                                                                                 \
    __asm__ volatile("mov %%r9, %0" : "=r"(name)::)

/**
 * @brief Process termination handler
 * @param current Pointer to current process control block
 * @param status Process exit status
 *
 * Detailed execution flow:
 * 1. Memory Management:
 *    - Adds process memory to free stack for reuse
 *    - PROCESS_MEM_FREE_STACK[0] contains stack pointer
 *    - PROCESS_MEM_FREE_STACK[1..N] contain free indices
 *
 * 2. Process Scheduling:
 *    - schedule_end() performs:
 *      a. Removes process from scheduler queue
 *      b. Selects next process to run
 *      c. Returns new current process pointer
 *
 * 3. Context Preparation:
 *    - Loads new process's page table into R12 (for context switch)
 *    - Loads new stack pointer into R11 (for context restore)
 *
 * 4. Cleanup:
 *    - Releases terminal control (tty_endProcess)
 *    - Updates framebuffer to reflect process change
 *
 * Note: Uses inline ASM to prepare registers for context_switch.S
 */
void sys_exit()
{
    uint64_t status;
    __asm__ volatile("mov %%rdi, %0" : "=r"(status)::);

    /* Add process memory to free stack */
    PROCESS_MEM_FREE_STACK[++PROCESS_MEM_FREE_STACK[0]] = (*CURRENT_PROCESS)->pid;

    /* Schedule next process and get new current */
    (*CURRENT_PROCESS) = schedule_end((*CURRENT_PROCESS));

    /* Prepare for context switch:
     * R12 = new process's page table root (CR3)
     * R11 = new process's stack pointer */
    __asm__ volatile("mov %0, %%r12\n\t" ::"r"((*CURRENT_PROCESS)->page_table->pml4) :);
    __asm__ volatile("mov %0, %%r11\n\t" ::"r"(&(*CURRENT_PROCESS)->process_stack_signature) :);
    TSS->ist1 =
        (uint64_t)(&(*CURRENT_PROCESS)->process_stack_signature) + sizeof(process_stack_layout_t);

    /* Terminal and display cleanup */
    // tty_endProcess(FBCON_TTY); /* Release terminal */
    // fbcon_update();            /* Update framebuffer */
}

/**
 * @brief Output writing implementation
 * @param out File descriptor (1=stdout)
 * @param msg User-space message pointer
 * @param len Message length in bytes
 *
 * Memory Safety:
 * - The (char*)msg is offset by ADDRESS_SECTION_SIZE*(2+index) because:
 *   - Each process has isolated memory sections
 *   - Section 0: Kernel
 *   - Section 1: Shared
 *   - Section 2+N: Per-process memory
 *   - This calculates the proper virtual address offset
 *
 * Output Handling:
 * - Only stdout (FD 1) is currently implemented
 * - Writes go to the process's controlling terminal
 * - Forces framebuffer update for immediate visibility
 */
void sys_write()
{
    uint64_t out, msg, len;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     "mov %%rdx, %2\n\t"
                     : "=r"(out), "=r"(msg), "=r"(len)::"rdi", "rsi", "rdx");
    if (out == 1) /* stdout */
    {
        /* Calculate proper virtual address offset for process memory */
        dev_kernel_fn(VCONS[0].dev_id, DEV_WRITE,
                      (ADDRESS_SECTION_SIZE * (2 + (*CURRENT_PROCESS)->pid)) + (char*)msg, len);
        /* Write to terminal output stream */
    }
    /* TODO: Implement stderr (FD 2) and other file descriptors */
}

/**
 * @brief Terminal input implementation
 * @param out File descriptor (1=stdout)
 * @param msg User-space message pointer
 * @param len Message length in bytes
 */
void sys_input()
{
    uint64_t in, msg, len;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     "mov %%rdx, %2\n\t"
                     : "=r"(in), "=r"(msg), "=r"(len)::"rdi", "rsi", "rdx");

    if (in == 1) /* stdout */
    {
        /* Calculate proper virtual address offset for process memory */

        dev_kernel_fn(VCONS[0].dev_id, DEV_READ,
                      (ADDRESS_SECTION_SIZE * (2 + (*CURRENT_PROCESS)->pid)) + (char*)msg, len);
    }
    /* TODO: Implement stderr (FD 2) and other file descriptors */
}

void execve()
{

    uint64_t name;
    __asm__ volatile("mov %%rdi, %0\n\t" : "=r"(name)::"rdi");

    directory_t* directory;
    filesystem_findDirectory(ROOT, &directory, "bin");

    for (int i = 0; i < directory->entry_count; i++)
    {
        filesystem_entry_t* entry = directory->entries[i];
        if (entry->file_type == EXT2_FT_REG_FILE && kernel_strcmp(entry->file.name, "shell") == 0)
        {
            page_table_t* table = pageTable_createPageTable();
            elfLoader_load(table, 0, &entry->file.file);
        }
    }
    LOG_VARIABLE(directory->entry_count, "r15");
    BREAKPOINT;
}