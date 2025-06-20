#include <drivers/fbcon.h>
#include <drivers/vcon.h>
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
        dev_kernel_fn(FBCON->dev_id, 1, 1, 0);
        vcon->vcon_line--;
    }
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
        VCONS[i].dev_id = dev_create(name, 0);

        /* Registers Callbacks */
        dev_register_kernel_callback(VCONS[i].dev_id, DEV_WRITE, vcon_write);
        dev_register_kernel_callback(VCONS[i].dev_id, DEV_READ, vcon_input);
    }
}

void vcon_keyboard_handle(key_event_t key)
{
    vcon_t* vcon = &VCONS[0];
    if (!vcon->cononical)
        return;

    /* Cononical/Print */
    if (key.keycode == '\n' || key.keycode == '\b' || (key.keycode >= 32 && key.keycode <= 126))
    {
        switch (key.keycode)
        {
        case '\n': /* Handles new line */
            vcon->cononical = false;
            vcon->input_buffer[vcon->input_buffer_pointer] = 0;
            schedule_unblock(vcon->input_block_process);
            vcon_putc('\n');
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
            dev_kernel_fn(FBCON->dev_id, 0, ' ',
                          ((uint64_t)vcon->vcon_column << 32) | vcon->vcon_line);
            break;
        default:
            dev_kernel_fn(FBCON->dev_id, 0, key.keycode,
                          ((uint64_t)vcon->vcon_column++ << 32) | vcon->vcon_line);
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
        if (dev_kernel_fn(keyboard_get_dev()->dev_id, DEV_READ, &key, sizeof(key_event_t)) ==
                sizeof(key_event_t) &&
            key.pressed)
        {
            vcon_keyboard_handle(key);
        }
    }
}

void vcon_putc(char c)
{
    vcon_t* vcon = &VCONS[0];

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
            dev_kernel_fn(FBCON->dev_id, 0, c,
                          ((uint64_t)vcon->vcon_column++ << 32) | vcon->vcon_line);
            vcon_handle_cursor(vcon);
            break;
        }
    }
}

size_t vcon_write(const char* str, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (!str[i])
            return i;
        vcon_putc(str[i]);
    }
    return size;
}

size_t vcon_input(const char* str, size_t size)
{
    schedule_block(*CURRENT_PROCESS);

    /* Allows writing in the terminal */
    VCONS[0].cononical = true;
    VCONS[0].input_buffer_pointer = 0;
    VCONS[0].input_block_process = (*CURRENT_PROCESS);
    VCONS[0].input_buffer = str;

    (*CURRENT_PROCESS) = scheduler_nextProcess();

    /* Prepare for context switch:
     * R12 = new process's page table root (CR3)
     * R11 = new process's stack pointer */
    INTERRUPT_INFO->cr3 = (*CURRENT_PROCESS)->page_table->pml4;
    INTERRUPT_INFO->rsp = &(*CURRENT_PROCESS)->process_stack_signature;
    TSS->ist1 =
        (uint64_t)(&(*CURRENT_PROCESS)->process_stack_signature) + sizeof(process_stack_layout_t);

    return 0;
}