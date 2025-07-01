/* elfLoader.h */
#ifndef ELF_LOAD_H
#define ELF_LOAD_H

#include <kernel/process.h>
#include <memory/pageTable.h>

typedef struct file_descriptor_t file_descriptor_t;

int elfLoader_systemd(file_descriptor_t* file);

int elfLoader_load(page_table_t* pageTable, file_descriptor_t* file, process_t* process);

#endif