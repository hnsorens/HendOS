#ifndef VCON_H
#define VCON_H

#include <kernel/process.h>
#include <kint.h>

#define VCON_COUNT 128

typedef struct vcon_t
{
    uint64_t vcon_column;
    uint64_t vcon_line;
    uint64_t dev_id;
    bool cononical;

    uint64_t input_buffer_pointer;
    process_t* input_block_process;
    char* input_buffer;
} vcon_t;

void vcon_init();

void vcon_putc(char c);

size_t vcon_write(const char* str, size_t size);

size_t vcon_input(const char* str, size_t size);

void vcon_handle_user_input();

#endif