/**
 * @file elfLoader.h
 * @brief ELF Loader Interface
 *
 * Declares functions for loading ELF binaries and setting up process address spaces during kernel boot.
 */
#ifndef ELF_LOAD_H
#define ELF_LOAD_H

#include <kernel/process.h>
#include <memory/pageTable.h>

typedef struct file_descriptor_t file_descriptor_t;

int elfLoader_systemd(file_descriptor_t* file);

int elfLoader_load(page_table_t* pageTable, file_descriptor_t* file, process_t* process);

#endif