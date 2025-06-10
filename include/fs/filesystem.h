/* filesystem.h */
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <drivers/ext2.h>
#include <fs/stream.h>
#include <kint.h>

typedef struct file_t
{
    ext2_file_t file;
    struct directory_t* dir;
    char* name;
    char* path;
} file_t;

typedef long (*device_read_callback)(void* ctx, void* buffer, long size);
typedef long (*device_write_callback)(void* ctx, void* buffer, long size);

typedef struct
{
    uint64_t dev_id;
    struct directory_t* dir;
    char* name;
    char* path;
} dev_file_t;

typedef struct directory_t
{
    ext2_dirent_iter_t iter;
    uint32_t entry_count;
    struct filesystem_entry_t** entries;
    struct directory_t* last;
    char* name;
    char* path;
} directory_t;

typedef struct filesystem_entry_t
{
    uint8_t file_type;
    union
    {
        directory_t dir;
        file_t file;
        dev_file_t dev_file;
    };
} filesystem_entry_t;

#define IDE_CMD_REG 0x1F7
#define IDE_STATUS_REG 0x1F7
#define IDE_DATA_REG 0x1F0

void filesystem_init();
void filesystem_populateDirectory(directory_t* dir);

dev_file_t* filesystem_createDevFile(const char* filename, uint64_t dev_flags);
file_t filesystem_createFile(directory_t* dir, const char* filename);
directory_t filesystem_createDir(directory_t* dir, const char* directoryname);
int filesystem_findDirectory(directory_t* current_dir, directory_t** out, char* dirname);
int filesystem_findParentDirectory(directory_t* current_dir, directory_t** out, char* dirname);
uint8_t filesystem_validDirectoryname(char* dirname);
uint8_t filesystem_validFilename(char* filename);

#endif /* FILESYSTEM_H */