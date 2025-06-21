#ifndef FILE_DESCRIPTOR_MANAGER_H
#define FILE_DESCRIPTOR_MANAGER_H

#include <drivers/ext2.h>
#include <fs/vfs.h>
#include <kint.h>

typedef uint64_t (*file_ops_t)(uint64_t arg1, uint64_t arg2);

// TODO: Take open_file_t out of the VFS_entry, then make them share an inode pointer

typedef struct open_file_t
{
    void* file_data;
    uint32_t inode_num;
    size_t pos;
    ext2_inode* inode;
    file_ops_t* ops; // TODO: get this from the VFS
    uint64_t offset;
    uint64_t refcount;
    uint64_t mode;
} open_file_t;

typedef struct file_descriptor_t
{
    open_file_t* open_file;
    int flags;
} file_descriptor_t;

typedef struct vfs_entry_t vfs_entry_t;

open_file_t* fdm_open_file(vfs_entry_t* current);

#endif