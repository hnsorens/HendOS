/* elfLoader.h */
#ifndef ELF_LOAD_H
#define ELF_LOAD_H

#include <kernel/process.h>
#include <memory/pageTable.h>

typedef struct open_file_t open_file_t;

int elfLoader_systemd(open_file_t* file);

int elfLoader_load(page_table_t* pageTable, open_file_t* file, process_t* process);

#endif