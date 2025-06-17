#include <stdint.h>
#include <unistd.h>

int execve(const char* name, int argc, char** argv)
{
    __asm__ volatile("mov $2, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "mov %2, %%rdx\n\t"
                     "int $0x80\n\t" ::"r"((uint64_t)name),
                     "r"((uint64_t)argc), "r"((uint64_t)argv)
                     : "rax", "rdi", "rsi", "rdx");
}

int execvp(const char* name, int argc, char** argv)
{
    __asm__ volatile("mov $9, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "mov %2, %%rdx\n\t"
                     "int $0x80\n\t" ::"r"((uint64_t)name),
                     "r"((uint64_t)argc), "r"((uint64_t)argv)
                     : "rax", "rdi", "rsi", "rdx");
}

int fork(void)
{
    __asm__ volatile("mov $8, %%rax\n\t"
                     "int $0x80\n\t" ::
                         : "rax");
    uint64_t var;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(var)::"rax");
    return var;
}

int getpgid(uint64_t pid)
{
    __asm__ volatile("mov $10, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "int $0x80\n\t" ::"r"(pid)
                     : "rax", "rdi");
}

int setpgid(uint64_t pid, uint64_t pgid)
{
    __asm__ volatile("mov $11, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "int $0x80\n\t" ::"r"((uint64_t)pid),
                     "r"((uint64_t)pgid)
                     : "rax", "rdi", "rsi");
}