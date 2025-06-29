#ifndef VCON_H
#define VCON_H

#include <kernel/process.h>
#include <kint.h>

#define VCON_COUNT 128
#define CHRDEV_SETGRP 4
#define CHRDEV_GETGRP 5

typedef struct vcon_t
{
    uint64_t vcon_column;
    uint64_t vcon_line;
    uint64_t dev_id;
    bool cononical;
    uint64_t grp;

    uint64_t input_buffer_pointer;
    process_t* input_block_process;
    char input_buffer[512];
    char* process_input_buffer;
    void* process_pml4;
} vcon_t;

void vcon_init();

void vcon_putc(vcon_t* vcon, char c);

size_t vcon_write(open_file_t* open_file, const char* str, size_t size);

size_t vcon_input(open_file_t* open_file, const char* str, size_t size);

void vcon_handle_user_input();

int vcon_setgrp(open_file_t* open_file, uint64_t pgid, uint64_t _1);
int vcon_getgrp(open_file_t* open_file, uint64_t _0, uint64_t _1);

#endif