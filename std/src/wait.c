#include <stdint.h>
#include <wait.h>

uint64_t waitpid(uint64_t pid, uint64_t* status, uint64_t options)
{
    __asm__ volatile("mov $17, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "mov %2, %%rdx\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((uint64_t)pid), "r"((uint64_t)status), "r"((uint64_t)options)
                     : "rax", "rdi", "rsi", "rdx");
    uint64_t var;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(var) : : "rax");
    return var;
}