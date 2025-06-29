/* stdio_impl.c - Minimal implementation of commonly used stdio functions */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Standard streams */
FILE* stdin = NULL;
FILE* stdout = NULL;
FILE* stderr = NULL;

/* Basic file operations */

FILE* fopen(const char* filename, const char* mode)
{
    __asm__ volatile("mov $12, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)filename), "r"((unsigned long)mode)
                     : "rax", "rdi", "rsi");
    FILE* stream;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(stream) : : "rax");
    return stream;
}

int fclose(FILE* stream)
{
    __asm__ volatile("mov $14, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)stream)
                     : "rax", "rdi");
    return 0;
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    __asm__ volatile("mov $3, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "mov %2, %%rdx\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)stream), "r"((unsigned long)ptr), "r"((unsigned long)(size * nmemb))
                     : "rax", "rdi", "rsi", "rdx");
    size_t bytes_read;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"((unsigned long)bytes_read) : : "rax");
    return bytes_read;
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    __asm__ volatile("mov $4, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "mov %2, %%rdx\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)stream), "r"((unsigned long)ptr), "r"((unsigned long)(size * nmemb))
                     : "rax", "rdi", "rsi", "rdx");
    size_t bytes_written;
    __asm__ volatile("mov %%rax, %0\n\t" : "=r"(bytes_written) : : "rax");
    return bytes_written;
}

int fseek(FILE* stream, long offset, int whence)
{
    __asm__ volatile("mov $21, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "mov %2, %%rdx\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)stream), "r"((unsigned long)offset), "r"((unsigned long)whence)
                     : "rax", "rdi", "rsi", "rdx");
    return 0;
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

int sprintf(char* str, const char* format, ...)
{
    /* TODO: Implement formatted output to string */
    return 0;
}

int snprintf(char* str, size_t size, const char* format, ...)
{
    /* TODO: Implement safe formatted output to string */
    return 0;
}

int fscanf(FILE* stream, const char* format, ...)
{
    /* TODO: Implement formatted input from file */
    return 0;
}

int scanf(const char* format, ...)
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

int sscanf(const char* str, const char* format, ...)
{
    /* TODO: Implement formatted input from string */
    return 0;
}

/* Character I/O */

int fgetc(FILE* stream)
{
    /* TODO: Implement character input */
    return EOF;
}

int getc(FILE* stream)
{
    /* TODO: Implement character input (may be macro in some implementations) */
    return fgetc(stream);
}

int getchar(void)
{
    /* TODO: Implement character input from stdin */
    return fgetc(stdin);
}

int fputc(int c, FILE* stream)
{
    /* TODO: Implement character output */
    return EOF;
}

int putc(int c, FILE* stream)
{
    /* TODO: Implement character output (may be macro in some implementations) */
    return fputc(c, stream);
}

int putchar(int c)
{
    /* TODO: Implement character output to stdout */
    return fputc(c, stdout);
}

/* String I/O */

char* fgets(char* s, int size, FILE* stream)
{
    __asm__ volatile("mov $3, %%rax\n\t"
                     "mov %0, %%rdi\n\t"
                     "mov %1, %%rsi\n\t"
                     "mov %1, %%rdx\n\t"
                     "int $0x80\n\t"
                     :
                     : "r"((unsigned long)stream), "r"((unsigned long)s), "r"((unsigned long)size)
                     : "rax", "rdi", "rsi", "rdx");
    return s;
}

int fputs(const char* s, FILE* stream)
{
    /* TODO: Implement string output */
    return EOF;
}

int puts(const char* s)
{
    /* TODO: Implement string output to stdout with newline */
    return EOF;
}

/* Error handling */

void clearerr(FILE* stream)
{
    /* TODO: Clear error indicators */
}

int feof(FILE* stream)
{
    /* TODO: Check end-of-file indicator */
    return 0;
}

int ferror(FILE* stream)
{
    /* TODO: Check error indicator */
    return 0;
}

void perror(const char* s)
{
    /* TODO: Print error message */
}

/* Initialization function (call this before using stdio) */
void __stdio_init(void)
{
    stdout = 0;
    stdin = 1;
    stderr = 2;
}