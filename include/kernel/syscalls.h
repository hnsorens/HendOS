/**
 * @file syscalls.h
 * @brief Kernel System Call Interface
 *
 * Defines the system call interface between user processes and the kernel.
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include <kernel/process.h>
#include <kint.h>

/* MSR constants */
#define IA32_STAR 0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_FMASK 0xC0000084

#define SYS_ARG_1(process) (process)->process_stack_signature.rdi
#define SYS_ARG_2(process) (process)->process_stack_signature.rsi
#define SYS_ARG_3(process) (process)->process_stack_signature.rdx
#define SYS_ARG_4(process) (process)->process_stack_signature.r10
#define SYS_ARG_5(process) (process)->process_stack_signature.r8
#define SYS_ARG_6(process) (process)->process_stack_signature.r9

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

#endif