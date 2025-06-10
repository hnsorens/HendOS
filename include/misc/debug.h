

#define BREAKPOINT while (1)

#define LOG_VARIABLE(var, reg) __asm__ volatile("mov %0, %%" reg : : "r"((uint64_t)(var)) :)