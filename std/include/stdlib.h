/**
 * @file stdlib.h
 */

#ifndef STD_LIB_H
#define STD_LIB_H

#include <stddef.h>
#include <stdint.h>

int atoi(const char* str);

char* itoa(uint32_t value, char* buffer, int base);

int rand(void);

void srand(unsigned int seed);

int abs(int x);

void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*));

__attribute__((noreturn)) void exit(int status);

void execve(const char* name, int argc, char** argv);

#endif