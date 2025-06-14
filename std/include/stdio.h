/**
 * @file stdio.h
 */

#ifndef STD_IO_H
#define STD_IO_H

#include <types.h>

typedef void FILE;

int printf(const char* format, ...);

int fgets(const char* str);

int scanf(const char* format, ...);

int chdir(const char* path);

int getcwd(const char* buffer, size_t size);

int putchar(int c);

int puts(const char* s);

int sprintf(char* str, const char* format, ...);

int snprintf(char* str, size_t size, const char* format, ...);

int getchar(void);

FILE* fopen(const char* filename, const char* mode);

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);

int fclose(FILE* stream);

#endif