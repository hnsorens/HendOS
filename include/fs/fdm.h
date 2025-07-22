/**
 * @file fdm.h
 * @brief File Descriptor Manager Interface
 *
 * Provides structures and functions for managing open files and file descriptors in the kernel.
 */

#ifndef FILE_DESCRIPTOR_MANAGER_H
#define FILE_DESCRIPTOR_MANAGER_H

#include <drivers/ext2.h>
#include <fs/vfs.h>
#include <kint.h>

#define FD_ENTRY_COUNT 32

/**
 * @struct file_descriptor_t
 * @brief Represents an open file instance
 *
 * Contains all metadata and operations for a file that is currently open,
 * including position tracking, reference counting, and file operations.
 */
typedef struct file_descriptor_t
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
} file_descriptor_t;

/**
 * @struct file_descriptor_entry_t
 * @brief An entry for 32 file descriptors
 *
 * An allocated array of 32 8 byte pointers, that can either point to a file descriptor
 * or another file_descriptor_entry_t depending on the depth
 */
typedef struct file_descriptor_entry_t
{
    file_descriptor_t* file_descriptors[FD_ENTRY_COUNT];
} file_descriptor_entry_t;

/* Forward declaration of VFS entry structure */
typedef struct vfs_entry_t vfs_entry_t;

/**
 * @brief Opens a file and creates an open file structure
 * @param current VFS entry representing the file to open
 * @return Pointer to newly created file_descriptor_t structure
 */
file_descriptor_t* fdm_open_file(vfs_entry_t* current);

/**
 * @brief Sets a file descriptor in a file_descriptor_entry_t
 * @param entry top level of file descriptor table
 * @param index fd element to set
 * @param fd file descriptor
 * @return returns 0 when completed successfully and -1 when failed
 */
int fdm_set(file_descriptor_entry_t* entry, size_t index, file_descriptor_t* fd);

/**
 * @brief Gets a file descriptor in a file_descriptor_entry_t
 * @param entry top level of file descriptor table
 * @param index fd element to set
 * @return Requested file descriptor or null if failed
 */
file_descriptor_t* fdm_get(file_descriptor_entry_t* entry, size_t index);

/**
 * @brief Copies the entries in a file descriptor table
 * @param src top of source file descriptor table
 * @param dst top of destination file descriptor table
 * @return 0 on success, -1 on failure
 */
int fdm_copy(file_descriptor_entry_t* src, file_descriptor_entry_t* dst);

/**
 * @brief Frees an entire file descriptor table, used on process cleanup
 * @param entry top of file descriptor entry
 */
void fdm_free(file_descriptor_entry_t* entry);

#endif /* FILE_DESCRIPTOR_MANAGER_H */