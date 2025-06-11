#ifndef VCON_H
#define VCON_H

#include <kint.h>

#define VCON_COUNT 128

typedef struct vcon_t
{
    uint64_t vcon_column;
    uint64_t vcon_line;
    uint64_t dev_id;
    bool cononical;
} vcon_t;

void vcon_init();

void vcon_putc(char c);

size_t vcon_write(const char* str, size_t size);

size_t vcon_input(const char* str, size_t size);

#endif