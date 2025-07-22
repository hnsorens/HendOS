#include <errno.h>
#include <stdarg.h>
#include <syscall.h>

long __syscall_ret(unsigned long r)
{
    if (r > -4096UL)
    {

        return -1;
    }
    return r;
}

long syscall(long number, ...)
{
    va_list ap;
    va_start(ap, number);
    long a = va_arg(ap, long);
    long b = va_arg(ap, long);
    long c = va_arg(ap, long);
    long d = va_arg(ap, long);
    long e = va_arg(ap, long);
    long f = va_arg(ap, long);
    va_end(ap);

    long ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(number), "D"(a), "S"(b), "d"(c), "r"(d), "r"(e), "r"(f) : "rcx", "r11", "memory");
    return __syscall_ret(ret);
}