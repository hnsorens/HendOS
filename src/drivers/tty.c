/* tty.c */
#include <drivers/keyboard.h>
#include <drivers/tty.h>
#include <fs/filesystem.h>
#include <kernel/device.h>
#include <kernel/syscalls.h>
#include <memory/kglobals.h>
#include <misc/debug.h>

/* In your TTY initialization */
void tty_show_welcome(tty_t* tty)
{
    // Static banner

    // Initial welcome message
    tty_add_row(tty, " _   _                _ _____ _____ ", TTY_ROW_REGULAR);
    tty_add_row(tty, "| | | |              | |  _  /  ___|", TTY_ROW_REGULAR);
    tty_add_row(tty, "| |_| | ___ _ __   __| | | | \\ `--. ", TTY_ROW_REGULAR);
    tty_add_row(tty, "|  _  |/ _ \\ '_ \\ / _` | | | |`--. \\", TTY_ROW_REGULAR);
    tty_add_row(tty, "| | | |  __/ | | | (_| \\ \\_/ /\\__/ /", TTY_ROW_REGULAR);
    tty_add_row(tty, "\\_| |_/\\___|_| |_|\\__,_|\\___/\\____/ ", TTY_ROW_REGULAR);
    tty_add_row(tty, "    HendOS v0.1.0 | Terminal Interface", TTY_ROW_REGULAR);
    tty_add_row(tty, "--------------------------------------------------", TTY_ROW_REGULAR);
    tty_add_row(tty, " Built on : May 11, 2025", TTY_ROW_REGULAR);
    tty_add_row(tty, " Arch     : x86_64", TTY_ROW_REGULAR);
    tty_add_row(tty, "--------------------------------------------------", TTY_ROW_REGULAR);
    tty_add_row(tty, " Type 'help' to begin.", TTY_ROW_REGULAR);
    tty_add_row(tty, NULL, TTY_ROW_REGULAR);
    tty_add_row(tty, NULL, TTY_ROW_REGULAR);
}

static void tty_add_row(tty_t* tty, const char* content, tty_row_type_t type)
{
    tty->dirty = true;
    /* Scroll if we've hit maximum rows */
    if (tty->row_count >= TTY_MAX_ROWS)
    {
        tty_scroll_up(tty);
    }

    tty_row_t* row = kmalloc(sizeof(tty_row_t));
    if (!row)
        return;

    kmemset(row, 0, sizeof(tty_row_t));
    row->type = type;

    if (content)
    {
        uint32_t len = 0;
        while (len < TTY_ROW_MAX_CHARS - 1 && content[len])
        {
            row->chars[len] = content[len];
            len++;
        }
        row->length = len;
    }

    /* Link into row list */
    if (!tty->head)
    {
        tty->head = tty->tail = row;
    }
    else
    {
        tty->tail->next = row;
        row->prev = tty->tail;
        tty->tail = row;
    }

    tty->current = row;
    tty->row_count++;
}

static void tty_remove_row(tty_t* tty)
{
    if (!tty->tail)
        return;
    tty->dirty = true;

    tty_row_t* prev = tty->tail->prev;
    kfree(tty->tail);

    if (prev)
    {
        prev->next = NULL;
        tty->tail = prev;
    }
    else
    {
        tty->head = tty->tail = NULL;
        tty_add_row(tty, NULL, TTY_ROW_REGULAR);
    }

    tty->row_count--;
}

static void tty_scroll_up(tty_t* tty)
{
    if (!tty->head)
        return;
    tty->dirty = true;

    tty_row_t* next = tty->head->next;
    kfree(tty->head);

    tty->head = next;
    if (tty->head)
    {
        tty->head->prev = NULL;
    }
    else
    {
        tty_add_row(tty, NULL, TTY_ROW_REGULAR);
    }

    tty->row_count--;
}

void tty_putchar(tty_t* tty, char ch)
{
    if (!tty->current)
        return;
    tty->dirty = true;

    /* Handle special characters */
    switch (ch)
    {
    case '\n':
        if (tty->canonical)
        {
            if (tty->is_input)
            {
                kmemcpy(tty->user_input, &tty->current->chars[tty->input_pointer],
                        tty->current->length - tty->input_pointer);
                schedule_unblock(tty->runningProcess);
            }
            else
            {
                tty_execute_command(tty);
            }
        }
        else
        {
            tty_add_row(tty, NULL, TTY_ROW_REGULAR);
        }
        return;

    case '\b':
        if (tty->input_pos > 0)
        {
            tty->current->length--;
            tty->current->chars[tty->current->length] = 0;
        }
        return;

    case '\t':
        /* Convert tab to spaces */
        for (int i = 0; i < 4; i++)
        {
            tty_putchar(tty, ' ');
        }
        return;
    }

    /* Normal character handling */
    if (tty->current->length < TTY_ROW_MAX_CHARS - 1)
    {
        tty->current->chars[tty->current->length++] = ch;
        tty->current->chars[tty->current->length] = 0;
    }
}

void tty_puts(tty_t* tty, const char* str)
{
    tty->dirty = true;
    while (*str)
    {
        tty_putchar(tty, *str++);
    }
}

void tty_SIGINT(tty_t* tty)
{
    if (!tty->runningProcess)
        return;
    sys_exit();
}

void tty_handle_key(tty_t* tty, uint8_t key, uint8_t modifier)
{
    tty->dirty = true;

    /* Translate keycode to character */
    char ch = key & 0xFF; /* Simple for now */

    /* Handle control keys */
    if (modifier & KEY_MOD_CTRL)
    {
        switch (ch)
        {
        case 'c': /* SIGINT */
            tty_puts(tty, "^C\n");
            tty_SIGINT(tty);
            return;
        case 'z': /* SIGSTP */
            tty_puts(tty, "^Z\n");
            return;
        case 'd': /* EOF */
            tty_puts(tty, "^D\n");
            return;
        case '/': /* SIGQUIT */
            tty_puts(tty, "^/\n");
            return;
        case 's': /* XOFF - flow */
            tty_puts(tty, "^S\n");
            return;
        case 'q': /* XON - flow */
            tty_puts(tty, "^Q\n");
            return;
        case 'l': /* Clears screen */
            tty_puts(tty, "^L\n");
            return;
        }
    }

    if ((key == '\n' || key == '\b' || (key >= 32 && key <= 126)) && tty->canonical)
    {
        /* Echo character if enabled */
        if (tty->echo)
        {
            tty_putchar(tty, ch);
        }

        /* Store in input buffer */
        if (tty->input_pos < TTY_INPUT_BUFFER_SIZE - 1)
        {
            if (ch == '\b')
            {
                if (tty->input_pos > 0)
                    tty->input_buffer[--tty->input_pos] = 0;
            }
            else
            {
                tty->input_buffer[tty->input_pos++] = ch;
                tty->input_buffer[tty->input_pos] = 0;
            }
        }
    }
}

void tty_endProcess(tty_t* tty)
{
    tty->runningProcess = 0;
    tty->processing = false;

    /* Reset input buffer */
    tty->input_pos = 0;

    /* Print new prompt */
    tty_print_prompt(tty);
}

void tty_execute_command(tty_t* tty)
{
    tty->dirty = true;
    if (tty->input_pos == 0)
    {
        tty_add_row(tty, NULL, TTY_ROW_REGULAR);
        tty_print_prompt(tty);
        return;
    }

    /* Null-terminate command */
    tty->input_buffer[tty->input_pos] = 0;

    /* Execute command */
    tty->processing = true;
    if (shell_execute(&tty->shell, tty->input_buffer))
    {
        tty_endProcess(tty);
    }
}

void tty_process_input(tty_t* tty)
{
    /* In a real system, this would process hardware input */
    /* For now, we assume keys are fed via tty_handle_key */
}

void tty_print_prompt(tty_t* tty)
{
    tty_add_row(tty, NULL, TTY_ROW_REGULAR);
    tty_puts(tty, "user@system:");
    tty_puts(tty, tty->shell.currentDir->path);
    tty_puts(tty, "$ ");
    tty->input_pos = 0;
}

size_t tty_print(char* str, size_t size)
{
    FBCON_TTY->dirty = true;
    for (int i = 0; i < size; i++)
    {
        if (*str == '\n')
        {
            tty_add_row(FBCON_TTY, NULL, TTY_ROW_REGULAR);
            *str++;
        }
        else
        {
            tty_putchar(FBCON_TTY, *str++);
        }
    }
    return size;
}

size_t tty_input(char* str, size_t size)
{
    /* Blocks current process until input is complete */
    schedule_block((*CURRENT_PROCESS));

    (*CURRENT_PROCESS) = scheduler_nextProcess();

    /* Prepare for context switch:
     * R12 = new process's page table root (CR3)
     * R11 = new process's stack pointer */
    __asm__ volatile("mov %0, %%r12\n\t" ::"r"((*CURRENT_PROCESS)->page_table->pml4) :);
    __asm__ volatile("mov %0, %%r11\n\t" ::"r"(&(*CURRENT_PROCESS)->process_stack_signature) :);
    TSS->ist1 =
        (uint64_t)(&(*CURRENT_PROCESS)->process_stack_signature) + sizeof(process_stack_layout_t);

    /* Allows writing in the terminal */
    FBCON_TTY->input_pointer = FBCON_TTY->current->length;
    FBCON_TTY->is_input = true;
    FBCON_TTY->canonical = true;
    FBCON_TTY->user_input = str;

    return 0;
}

void tty_setProcess(tty_t* tty, process_t* process)
{
    tty->runningProcess = process;
}

void tty_init(tty_t* tty, bool show_welcome)
{
    memset(tty, 0, sizeof(tty_t));

    /* Initialize default terminal settings */
    tty->echo = true;
    tty->canonical = true;
    tty->processing = false;
    tty->dirty = true;
    tty->runningProcess = 0;
    tty->is_input = false;

    /* Create TTY device */
    tty->dev = filesystem_createDevFile("tty", 0);

    /* Adds tty callbacks */
    dev_register_kernel_callback(tty->dev->dev_id, DEV_WRITE, tty_print);
    dev_register_kernel_callback(tty->dev->dev_id, DEV_READ, tty_input);

    /* Create first empty row */
    tty_add_row(tty, NULL, TTY_ROW_REGULAR);

    /* Initialize shell */
    shell_init(&tty->shell, tty->dev);

    /* Print prompt */
    if (show_welcome)
        tty_show_welcome(tty);
    tty_print_prompt(tty);
}