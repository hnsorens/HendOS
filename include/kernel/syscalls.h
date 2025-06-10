/**
 * @file syscalls.h
 * @brief Kernel System Call Interface
 *
 * Defines the system call interface between user processes and the kernel.
 * System calls provide controlled access to kernel functionality and hardware.
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include <kernel/process.h>
#include <kint.h>

/* MSR constants */
#define IA32_STAR 0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_FMASK 0xC0000084

/**
 * @brief Inline asm helpers for MSR read/write
 */
static inline void wrmsr(uint32_t msr, uint64_t value);

/* Read CR4 register */
static inline uint64_t read_cr4();

/* Write CR4 register */
static inline void write_cr4(uint64_t val);

void syscall_handler(void);

/* ================================ Public API ============================= */

/**
 * @brief Initializes all syscalls
 */
void syscall_init();

/* ================================ SYSCALL API =================================*/

/**
 * @brief Terminate the calling process
 * @param arg1 Exit status code (unix-style return value)
 *
 * Performs:
 * 1. Process cleanup (memory deallocation, resource release)
 * 2. Process table updates
 * 3. Context switch to next process
 *
 * Note: This is the syscall implementation, not the userspace stub
 */
void sys_exit();

/**
 * @brief Write output to a file descriptor
 * @param arg1 File descriptor (1=stdout, 2=stderr)
 * @param arg2 Pointer to message buffer in process memory
 * @param arg3 Length of message in bytes
 *
 * Supported outputs:
 * - FD 1 (stdout): Writes to process's terminal output
 * - FD 2 (stderr): Currently same as stdout
 *
 * Security checks:
 * - Validates msg is within process memory space
 * - Checks for valid file descriptor
 */
void sys_write();

/**
 * @brief Terminal input implementation
 * @param out File descriptor (1=stdout)
 * @param msg User-space message pointer
 * @param len Message length in bytes
 */
void sys_input();

void open();

void read();

void write();

void close();

#endif