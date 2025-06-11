#include <drivers/vcon.h>
#include <kernel/device.h>
#include <memory/kglobals.h>

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
    if (vcon->vcon_column == VCON_WIDTH)
    {
        vcon->vcon_column = 0;
        vcon->vcon_line++;
    }
    if (vcon->vcon_line == VCON_HEIGHT)
    {
        vcon->vcon_top++;
    }
    if (vcon->vcon_top == VCON_HEIGHT)
    {
        vcon->vcon_top = 0;
    }
}

void vcon_init()
{
    char name[] = "vcon   ";
    for (size_t i = 0; i < VCON_COUNT; i++)
    {
        /* Initializes vcon structure */
        VCONS[i].cononical = false;
        VCONS[i].vcon_top = 0;
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

void vcon_putc(char c)
{
    vcon_t* vcon = &VCONS[0];

    /* Implement SIGS */

    /* Cononical/Print */
    if (c != '\n' || c == '\b' || (c >= 32 && c <= 126))
    {
        switch (c)
        {
        case '\n': /* Handles new line */
            vcon->vcon_column = VCON_WIDTH;
            vcon_handle_cursor(vcon);
            break;
        case '\b': /* Handles backspace */
            if (vcon->vcon_column == 0)
            {
                /* Cant go off the top of the page */
                if (vcon->vcon_line != vcon->vcon_top)
                {
                    vcon->vcon_column = VCON_WIDTH - 1;
                    if (vcon->vcon_line == 0)
                    {
                        vcon->vcon_column = VCON_HEIGHT - 1;
                    }
                    else
                    {
                        vcon->vcon_column--;
                    }
                }
            }
            else
            {
                /* Goes back 1 and makes it a white space */
                vcon->vcon_buffer[vcon->vcon_line][--vcon->vcon_column] = ' ';
            }
            break;
        default:
            vcon->vcon_buffer[vcon->vcon_line][vcon->vcon_column++] = c;
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

    (*CURRENT_PROCESS) = scheduler_nextProcess();

    /* Prepare for context switch:
     * R12 = new process's page table root (CR3)
     * R11 = new process's stack pointer */
    __asm__ volatile("mov %0, %%r12\n\t" ::"r"((*CURRENT_PROCESS)->page_table->pml4) :);
    __asm__ volatile("mov %0, %%r11\n\t" ::"r"(&(*CURRENT_PROCESS)->process_stack_signature) :);
    TSS->ist1 =
        (uint64_t)(&(*CURRENT_PROCESS)->process_stack_signature) + sizeof(process_stack_layout_t);

    /* Allows writing in the terminal */
    VCONS[0].cononical = true;

    return 0;
}