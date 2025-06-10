/* tty.h - Improved TTY driver implementation */
#ifndef _TTY_H
#define _TTY_H

#include <kernel/scheduler.h>
#include <kernel/shell.h>
#include <memory/kmemory.h>
#include <stdbool.h>
#include <stdint.h>

#define TTY_MAX_ROWS 50
#define TTY_ROW_MAX_CHARS 256
#define TTY_INPUT_BUFFER_SIZE 512

typedef enum
{
    TTY_ROW_REGULAR,
    TTY_ROW_COMMAND,
    TTY_ROW_EXTRA
} tty_row_type_t;

typedef struct tty_row
{
    char chars[TTY_ROW_MAX_CHARS];
    uint32_t length;
    tty_row_type_t type;
    struct tty_row* next;
    struct tty_row* prev;
} tty_row_t;

typedef struct
{
    /* Terminal state */
    tty_row_t* head;
    tty_row_t* tail;
    tty_row_t* current;
    uint32_t row_count;

    /* Input handling */
    char input_buffer[TTY_INPUT_BUFFER_SIZE];
    uint32_t input_pos;

    /* Shell integration */
    shell_t shell;

    /* Terminal modes */
    bool echo;       /* Echo input characters */
    bool canonical;  /* Line-by-line input */
    bool processing; /* Prevent reentrancy */
    bool dirty;

    dev_file_t* dev;

    process_t* runningProcess;

    uint32_t input_pointer;
    uint32_t is_input;
    char* user_input;
} tty_t;

/* Public API */
void tty_init(tty_t* tty, bool show_welcome);
void tty_putchar(tty_t* tty, char ch);
void tty_puts(tty_t* tty, const char* str);
void tty_process_input(tty_t* tty);
void tty_handle_key(tty_t* tty, uint8_t key, uint8_t modifier);
void tty_print_prompt(tty_t* tty);
void tty_show_welcome(tty_t* tty);
void tty_endProcess(tty_t* tty);

/* Internal helpers */
static void tty_add_row(tty_t* tty, const char* content, tty_row_type_t type);
static void tty_remove_row(tty_t* tty);
static void tty_scroll_up(tty_t* tty);
static void tty_execute_command(tty_t* tty);

#endif /* _TTY_H */