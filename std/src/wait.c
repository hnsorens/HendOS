#include <sys/wait.h>

__pid_t waitpid(__pid_t __pid, int* __stat_loc, int __options)
{
    __asm__ volatile("mov $17, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "mov %2, %%rdx\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)__pid), "r"((unsigned long)__stat_loc), "r"((unsigned long)__options)
                     : "rax", "rdi", "rsi", "rdx");
    unsigned long var;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(var) : : "rax");
    return var;
}