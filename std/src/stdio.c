/**
 * @file stdio.c
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int printf(const char* format, ...)
{
    // Static output buffer
    char buffer[512];
    char temp[64]; // For individual converted values
    int pos = 0;

    va_list args;
    va_start(args, format);

    for (; *format; format++)
    {
        if (*format == '%')
        {
            format++;
            if (*format == 0)
                break;

            switch (*format)
            {
            case 's':
            {
                const char* s = va_arg(args, const char*);
                while (*s && pos < sizeof(buffer) - 1)
                {
                    buffer[pos++] = *s++;
                }
                break;
            }
            case 'd':
            {
                int val = va_arg(args, int);
                if (val < 0)
                {
                    buffer[pos++] = '-';
                    val = -val;
                }
                itoa((unsigned int)val, temp, 10);
                for (char* t = temp; *t && pos < sizeof(buffer) - 1; ++t)
                {
                    buffer[pos++] = *t;
                }
                break;
            }
            case 'u':
            {
                unsigned int val = va_arg(args, unsigned int);
                itoa(val, temp, 10);
                for (char* t = temp; *t && pos < sizeof(buffer) - 1; ++t)
                {
                    buffer[pos++] = *t;
                }
                break;
            }
            case 'x':
            {
                unsigned int val = va_arg(args, unsigned int);
                itoa(val, temp, 16);
                for (char* t = temp; *t && pos < sizeof(buffer) - 1; ++t)
                {
                    buffer[pos++] = *t;
                }
                break;
            }
            case 'c':
            {
                char c = (char)va_arg(args, int);
                buffer[pos++] = c;
                break;
            }
            case '%':
            {
                buffer[pos++] = '%';
                break;
            }
            default:
            {
                // Unknown format, just print it raw
                buffer[pos++] = '%';
                buffer[pos++] = *format;
                break;
            }
            }
        }
        else
        {
            buffer[pos++] = *format;
        }

        if (pos >= sizeof(buffer) - 1)
            break;
    }

    buffer[pos] = '\0';
    va_end(args);

    __asm__ volatile("mov $4, %%rax\n\t"
                     "mov $1, %%rdi\n\t"
                     "mov %0, %%rsi\n\t"
                     "mov %1, %%rdx\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)buffer), "r"((unsigned long)pos)
                     : "rax", "rdi", "rsi", "rdx");
    return pos;
}

int fgets(const char* str)
{
    __asm__ volatile("mov $3, %%rax\n\t"
                     "mov $1, %%rdi\n\t"
                     "mov %0, %%rsi\n\t"
                     "mov %1, %%rdx\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)str), "r"((unsigned long)512)
                     : "rax", "rdi", "rsi", "rdx");
    return 0;
}

int scanf(const char* fmt, ...)
{
    char input[512];
    __asm__ volatile("mov $3, %%rax\n\t"
                     "mov $1, %%rdi\n\t"
                     "mov %0, %%rsi\n\t"
                     "mov %1, %%rdx\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)input), "r"((unsigned long)512)
                     : "rax", "rdi", "rsi", "rdx");

    va_list args;
    va_start(args, fmt);

    const char* in = input;
    const char* f = fmt;
    int assigned = 0;

    while (*f)
    {
        if (*f == '%')
        {
            f++;
            switch (*f)
            {
            case 'd':
            {
                int* iptr = va_arg(args, int*);
                while (*in == ' ')
                    in++; // skip spaces
                *iptr = atoi(in);
                while (*in && *in != ' ' && *in != '\n')
                    in++;
                assigned++;
                break;
            }
            case 's':
            {
                char* sptr = va_arg(args, char*);
                while (*in == ' ')
                    in++;
                while (*in && *in != ' ' && *in != '\n')
                {
                    *sptr++ = *in++;
                }
                *sptr = '\0';
                assigned++;
                break;
            }
            case 'c':
            {
                char* cptr = va_arg(args, char*);
                while (*in == ' ')
                    in++;
                *cptr = *in ? *in++ : 0;
                assigned++;
                break;
            }
            default:
                break;
            }
        }
        f++;
    }

    va_end(args);
    return assigned;
}

int chdir(const char* path)
{
    __asm__ volatile("mov $5, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)path)
                     : "rax", "rdi");
}

int getcwd(const char* buffer, size_t size)
{
    __asm__ volatile("mov $6, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)buffer), "r"((unsigned long)size)
                     : "rax", "rdi", "rsi");
}

FILE* fopen(const char* filename, const char* mode)
{
    __asm__ volatile("mov $12, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)filename), "r"((unsigned long)mode)
                     : "rax", "rdi", "rsi");
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {}

int fclose(FILE* stream) {}