/* shell.h */
#ifndef SHELL_H
#define SHELL_H

#include <fs/filesystem.h>
#include <kint.h>

typedef struct
{
    char** args;
    int count;
} arg_list_t;

typedef struct
{
    directory_t* currentDir;
    dev_file_t* tty_dev;
} shell_t;

void shell_init(shell_t* shell, dev_file_t* dev);
int shell_execute(shell_t* shell, char* message);

#endif