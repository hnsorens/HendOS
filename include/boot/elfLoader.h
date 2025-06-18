/* elfLoader.h */
#ifndef ELF_LOAD_H
#define ELF_LOAD_H

#include <fs/filesystem.h>
#include <kernel/process.h>
#include <memory/pageTable.h>

int elfLoader_systemd(page_table_t* pageTable, file_t* file);

void elfLoader_load(page_table_t* pageTable, file_t* file, process_t* process);

#endif