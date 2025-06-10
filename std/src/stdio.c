/**
 * @file stdio.c
 */

#include <stdarg.h>
#include <stdio.h>
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
