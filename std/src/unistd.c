#include <unistd.h>

int dup2(int __fd, int __fd2)
{
    __asm__ volatile("mov $13, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)__fd), "r"((unsigned long)__fd2)
                     : "rax", "rdi", "rsi");
    unsigned long var;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(var) : : "rax");
    return var;
}

ssize_t read(int __fd, void* __buf, size_t __nbytes)
{
    /* TODO: Implement read */
    return 0;
}

ssize_t write(int __fd, const void* __buf, size_t __n)
{
    /* TODO: Implement write */
    return 0;
}

int close(int __fd)
{
    /* TODO: implement close */
    return 0;
}

__pid_t fork(void)
{
    __asm__ volatile("mov $8, %%rax\n\t"
                     "int $0x80\n\t"
                     :
                     :
                     : "rax");
    unsigned long var;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(var) : : "rax");
    return var;
}

__pid_t getpid(void) {}

int setpgid(__pid_t __pid, __pid_t __pgid)
{
    __asm__ volatile("mov $11, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)__pid), "r"((unsigned long)__pgid)
                     : "rax", "rdi", "rsi");
    unsigned long var;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(var) : : "rax");
    return var;
}

__pid_t getsid(__pid_t __pid)
{
    __asm__ volatile("mov $19, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)__pid)
                     : "rax", "rdi");
    unsigned long var;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(var) : : "rax");
    return var;
}

int execve(const char* __path, char* const __argv[], char* const __envp[])
{
    __asm__ volatile("mov $2, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "mov %2, %%rdx\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)__path), "r"((unsigned long)__argv), "r"((unsigned long)sizeof(__argv))
                     : "rax", "rdi", "rsi", "rdx");
    unsigned long var;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(var) : : "rax");
    return var;
}

unsigned int sleep(unsigned int __seconds)
{
    return 0;
}

int pipe(int __pipedes[2])
{
    return 0;
}

int tcsetpgrp(int __fd, __pid_t __pgrp_id)
{
    __asm__ volatile("mov $15, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)__fd), "r"((unsigned long)__pgrp_id)
                     : "rax", "rdi", "rsi");
    unsigned long var;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(var) : : "rax");
    return var;
}

__pid_t tcgetpgrp(int __fd)
{
    __asm__ volatile("mov $16, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)__fd)
                     : "rax", "rdi");
    unsigned long var;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(var) : : "rax");
    return var;
}

char* getcwd(char* __buf, size_t __size)
{
    __asm__ volatile("mov $6, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)__buf), "r"((unsigned long)__size)
                     : "rax", "rdi", "rsi");
    return __buf;
}

int chdir(const char* __path)
{
    __asm__ volatile("mov $5, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)__path)
                     : "rax", "rdi");
    unsigned long var;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(var) : : "rax");
    return var;
}