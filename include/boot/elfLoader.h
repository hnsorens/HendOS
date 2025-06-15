/* elfLoader.h */
#ifndef ELF_LOAD_H
#define ELF_LOAD_H

#include <fs/filesystem.h>
#include <kernel/shell.h>
#include <memory/pageTable.h>

int elfLoader_load(page_table_t* pageTable, shell_t* shell, file_t* file, int argc, char** argv);

#endif