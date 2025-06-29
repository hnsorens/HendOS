/**
 * @file file_descriptor_manager.h
 * @brief File Descriptor Management Interface
 *
 * Provides structures and functions for managing open files and file descriptors
 * in the kernel. Handles file operations tracking, reference counting, and
 * descriptor-to-file mapping.
 */

#ifndef FILE_DESCRIPTOR_MANAGER_H
#define FILE_DESCRIPTOR_MANAGER_H

#include <drivers/ext2.h>
#include <fs/vfs.h>
#include <kint.h>

/**
 * @struct open_file_t
 * @brief Represents an open file instance
 *
 * Contains all metadata and operations for a file that is currently open,
 * including position tracking, reference counting, and file operations.
 */
typedef struct open_file_t
{
    uint32_t inode_num;
    size_t pos;
    ext2_inode* inode;
    file_ops_t* ops;
    uint64_t offset;
    uint64_t refcount;
    uint64_t mode;
    uint8_t type;
    void* private_data;
} open_file_t;

/**
 * @struct file_descriptor_t
 * @brief Represents a file descriptor entry
 *
 * Maps a file descriptor number to an open file and contains descriptor-specific
 * flags and state information.
 */
typedef struct file_descriptor_t
{
    open_file_t* open_file;
    int flags;
} file_descriptor_t;

/* Forward declaration of VFS entry structure */
typedef struct vfs_entry_t vfs_entry_t;

/**
 * @brief Opens a file and creates an open file structure
 * @param current VFS entry representing the file to open
 * @return Pointer to newly created open_file_t structure
 */
open_file_t* fdm_open_file(vfs_entry_t* current);

#endif /* FILE_DESCRIPTOR_MANAGER_H */