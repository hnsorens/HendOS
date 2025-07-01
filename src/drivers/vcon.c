#include <drivers/fbcon.h>
#include <drivers/vcon.h>
#include <fs/fdm.h>
#include <fs/vfs.h>
#include <kernel/device.h>
#include <kernel/scheduler.h>
#include <kstring.h>
#include <memory/kglobals.h>
#include <misc/debug.h>

char* itoa(unsigned int num, char* buf)
{
    char* p = buf;
    char* start;
    unsigned int temp = num;
    int digit_count = 0;

    /* Handle zero explicitly */
    if (num == 0)
    {
        *p++ = '0';
        *p = '\0';
        return buf;
    }

    /* Count digits */
    while (temp > 0)
    {
        temp /= 10;
        digit_count++;
    }

    p += digit_count; /* move pointer to the end of the string area */
    *p = '\0';        /* null-terminate */

    start = p - digit_count; /* start of digits */

    /* Fill digits from the end */
    while (num > 0)
    {
        *(--p) = (num % 10) + '0';
        num /= 10;
    }

    return start;
}

static void vcon_handle_cursor(vcon_t* vcon)
{
    if (vcon->vcon_column == FBCON_GRID_WIDTH)
    {
        vcon->vcon_column = 0;
        vcon->vcon_line++;
    }
    if (vcon->vcon_line == FBCON_GRID_HEIGHT)
    {
        FBCON->fbcon->ops[5](FBCON->fbcon, 1, 0);
    }
}

int vcon_setgrp(file_descriptor_t* open_file, uint64_t pgid, uint64_t _1)
{
    vcon_t* vcon = open_file->private_data;
    vcon->grp = pgid;
    return pgid;
}

int vcon_getgrp(file_descriptor_t* open_file, uint64_t _0, uint64_t _1)
{
    vcon_t* vcon = open_file->private_data;
    return vcon->grp;
}

void vcon_init()
{
    char name[] = "vcon   ";
    for (size_t i = 0; i < VCON_COUNT; i++)
    {
        /* Initializes vcon structure */
        VCONS[i].cononical = false;
        VCONS[i].vcon_line = 0;
        VCONS[i].vcon_column = 0;

        /* Creates device object */
        itoa(i, name + 4);

        vfs_entry_t* device_file = vfs_create_entry(*DEV, name, EXT2_FT_CHRDEV);

        device_file->ops[DEV_WRITE] = vcon_write;
        device_file->ops[DEV_READ] = vcon_input;
        device_file->ops[CHRDEV_SETGRP] = vcon_setgrp;
        device_file->ops[CHRDEV_GETGRP] = vcon_getgrp;
        device_file->private_data = &VCONS[i];
    }
}

void vcon_keyboard_handle(vcon_t* vcon, key_event_t key)
{

    if (key.modifiers & KEY_MOD_CTRL)
    {
        switch (key.keycode)
        {
        case 'c':
            FBCON->fbcon->ops[4](FBCON->fbcon, '^', ((uint64_t)vcon->vcon_column++ << 32) | vcon->vcon_line);
            vcon->input_buffer[vcon->input_buffer_pointer++] = key.keycode;
            vcon_handle_cursor(vcon);
            FBCON->fbcon->ops[4](FBCON->fbcon, 'C', ((uint64_t)vcon->vcon_column++ << 32) | vcon->vcon_line);
            vcon->input_buffer[vcon->input_buffer_pointer++] = key.keycode;
            vcon_handle_cursor(vcon);
            process_group_signal(vcon->grp, SIGINT);
            break;
        case '/':
            FBCON->fbcon->ops[4](FBCON->fbcon, '^', ((uint64_t)vcon->vcon_column++ << 32) | vcon->vcon_line);
            vcon->input_buffer[vcon->input_buffer_pointer++] = key.keycode;
            vcon_handle_cursor(vcon);
            FBCON->fbcon->ops[4](FBCON->fbcon, '/', ((uint64_t)vcon->vcon_column++ << 32) | vcon->vcon_line);
            vcon->input_buffer[vcon->input_buffer_pointer++] = key.keycode;
            vcon_handle_cursor(vcon);
            process_group_signal(vcon->grp, SIGQUIT);
            break;
        case 'z':
            FBCON->fbcon->ops[4](FBCON->fbcon, '^', ((uint64_t)vcon->vcon_column++ << 32) | vcon->vcon_line);
            vcon->input_buffer[vcon->input_buffer_pointer++] = key.keycode;
            vcon_handle_cursor(vcon);
            FBCON->fbcon->ops[4](FBCON->fbcon, 'Z', ((uint64_t)vcon->vcon_column++ << 32) | vcon->vcon_line);
            vcon->input_buffer[vcon->input_buffer_pointer++] = key.keycode;
            vcon_handle_cursor(vcon);
            process_group_signal(vcon->grp, SIGTSTP);
            break;

        default:
            break;
        }
    }

    if (!vcon->cononical)
        return;

    /* Cononical/Print */
    if (key.keycode == '\n' || key.keycode == '\b' || (key.keycode >= 32 && key.keycode <= 126))
    {
        switch (key.keycode)
        {
        case '\n': /* Handles new line */
            vcon->cononical = false;
            vcon->input_buffer[vcon->input_buffer_pointer++] = 0;
            uint64_t current_cr3;
            __asm__ volatile("mov %%cr3, %0\n\t" : "=r"(current_cr3) : :);
            __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(vcon->process_pml4) :);
            kmemcpy(vcon->process_input_buffer, vcon->input_buffer, vcon->input_buffer_pointer);
            __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(current_cr3) :);
            schedule_unblock(vcon->input_block_process);
            vcon_putc(vcon, '\n');
            break;
        case '\b': /* Handles backspace */
            if ((vcon->vcon_line != 0 || vcon->vcon_column != 0) && vcon->input_buffer_pointer != 0)
            {
                vcon->input_buffer_pointer--;
                if (vcon->vcon_column == 0)
                {
                    vcon->vcon_column = FBCON_GRID_WIDTH - 1;
                    vcon->vcon_line--;
                }
                else
                {
                    vcon->vcon_column--;
                }
            }
            FBCON->fbcon->ops[4](FBCON->fbcon, key.keycode, ((uint64_t)vcon->vcon_column << 32) | vcon->vcon_line);
            break;
        default:
            FBCON->fbcon->ops[4](FBCON->fbcon, key.keycode, ((uint64_t)vcon->vcon_column++ << 32) | vcon->vcon_line);
            vcon->input_buffer[vcon->input_buffer_pointer++] = key.keycode;
            vcon_handle_cursor(vcon);
            break;
        }
    }
}

void vcon_handle_user_input()
{
    while (keyboard_has_input())
    {
        key_event_t key;
        if (keyboard_get_dev()->ops[DEV_READ](FBCON->fbcon, &key, sizeof(key_event_t)) == sizeof(key_event_t) && key.pressed)
        {
            vcon_keyboard_handle(&VCONS[0], key);
        }
    }
}

void vcon_putc(vcon_t* vcon, char c)
{
    /* Cononical/Print */
    if (c == '\n' || c == '\b' || (c >= 32 && c <= 126))
    {
        switch (c)
        {
        case '\n': /* Handles new line */
            vcon->vcon_column = 0;
            vcon->vcon_line++;
            vcon_handle_cursor(vcon);
            break;
        case '\b': /* Handles backspace */
            if (vcon->vcon_line != 0 || vcon->vcon_column != 0)
            {
                if (vcon->vcon_column == 0)
                {
                    vcon->vcon_column = FBCON_GRID_WIDTH - 1;
                    vcon->vcon_line--;
                }
                else
                {
                    vcon->vcon_column--;
                }
            }
            break;
        default:
            FBCON->fbcon->ops[4](FBCON->fbcon, c, ((uint64_t)vcon->vcon_column++ << 32) | vcon->vcon_line);
            // vcon_handle_crusor(vcon);

            break;
        }
    }
}

size_t vcon_write(file_descriptor_t* open_file, const char* str, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (!str[i])
            return i;
        vcon_putc(open_file->private_data, str[i]);
    }
    return size;
}

size_t vcon_input(file_descriptor_t* open_file, const char* str, size_t size)
{
    vcon_t* vcon = open_file->private_data;

    /* Allows writing in the terminal */
    vcon->cononical = true;
    vcon->input_buffer_pointer = 0;
    vcon->input_block_process = (*CURRENT_PROCESS);
    vcon->process_input_buffer = str;
    vcon->process_pml4 = (*CURRENT_PROCESS)->page_table;

    schedule_block(*CURRENT_PROCESS);
    (*CURRENT_PROCESS) = scheduler_nextProcess();

    /* Prepare for context switch:
     * R12 = new process's page table root (CR3)
     * R11 = new process's stack pointer */
    INTERRUPT_INFO->cr3 = (*CURRENT_PROCESS)->page_table;
    INTERRUPT_INFO->rsp = &(*CURRENT_PROCESS)->process_stack_signature;
    TSS->ist1 = (uint64_t)(*CURRENT_PROCESS) + sizeof(process_stack_layout_t);

    return 0;
}