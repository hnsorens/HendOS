/**
 * @file memory.h
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

void* malloc(size_t size);

void free(void* ptr);

void* calloc(size_t nmemb, size_t size);

void* realloc(void* ptr, size_t size);

#endif