/**
 * @file debug.h
 * @brief Kernel Debugging Macros
 *
 * Provides macros for breakpoints and variable logging in kernel debugging.
 */

#define BREAKPOINT while (1)

#define LOG_VARIABLE(var, reg) __asm__ volatile("mov %0, %%" reg : : "r"((uint64_t)(var)) :)