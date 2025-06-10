/**
 * @file stdlib.c
 */

#include <stdlib.h>

int atoi(const char* str)
{
    int result = 0;
    int sign = 1;
    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    while (*str >= '0' && *str <= '9')
    {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result * sign;
}

char* itoa(uint32_t value, char* buffer, int base)
{
    const char* digits = "0123456789abcdef";
    char temp[32];
    int i = 0;

    if (value == 0)
    {
        temp[i++] = '0';
    }
    else
    {
        while (value > 0)
        {
            temp[i++] = digits[value % base];
            value /= base;
        }
    }

    // Reverse
    int j = 0;
    while (i > 0)
    {
        buffer[j++] = temp[--i];
    }
    buffer[j] = '\0';
}

int rand(void) {}

void srand(unsigned int seed) {}

int abs(int x) {}

void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*)) {}

void exit(uint64_t status)
{
    __asm__ volatile("mov $1, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "int $0x80\n\t" ::"r"(status)
                     :);
}