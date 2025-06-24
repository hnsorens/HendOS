#include <wait.h>

uint64_t waitpid(uint64_t pid)
{
    __asm__ volatile("mov $17, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"(pid)
                     : "rax", "rdi");
    uint64_t var;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(var) : : "rax");
    return var;
}