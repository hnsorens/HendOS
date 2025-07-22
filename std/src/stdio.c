/* stdio_impl.c - Minimal implementation of commonly used stdio functions */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>

/* Standard streams */
FILE* stdin = NULL;
FILE* stdout = NULL;
FILE* stderr = NULL;

/* Basic file operations */

FILE* fopen(const char* filename, const char* mode)
{
    return syscall(12, filename, mode);
}

int fclose(FILE* stream)
{
    return syscall(14, stream);
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    return syscall(3, stream, ptr, size * nmemb);
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    return syscall(4, stream, ptr, size * nmemb);
}

int fseek(FILE* stream, long offset, int whence)
{
    return syscall(21, stream, offset, whence);
}

long ftell(FILE* stream)
{
    /* TODO: Implement tell */
    return 0;
}

void rewind(FILE* stream)
{
    /* TODO: Implement rewind */
    fseek(stream, 0, SEEK_SET);
}

/* Formatted I/O */

int fprintf(FILE* stream, const char* format, ...)
{
    /* TODO: Implement formatted output to file */
    return 0;
}

static char* itoa(unsigned int value, char* buffer, int base)
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

    return buffer;
}

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
    syscall(4, 1, buffer, pos);
    return pos;
}

int scanf(const char* format, ...)
{
    char input[512];

    syscall(3, 1, input, 512);

    va_list args;
    va_start(args, format);

    const char* in = input;
    const char* f = format;
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

/* Character I/O */

int fgetc(FILE* stream)
{
    return EOF;
}

int getc(FILE* stream)
{
    return fgetc(stream);
}

int getchar(void)
{
    return fgetc(stdin);
}

int fputc(int c, FILE* stream)
{
    return EOF;
}

int putc(int c, FILE* stream)
{
    return fputc(c, stream);
}

int putchar(int c)
{
    return fputc(c, stdout);
}

char* fgets(char* s, int size, FILE* stream)
{
    syscall(3, stream, s, 512);
    return s;
}

void __stdio_init(void)
{
    stdout = 0;
    stdin = 1;
    stderr = 2;
}