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

/* Number of file descriptors per entry */
#define FD_ENTRY_COUNT 32

#include <drivers/ext2.h>
#include <fs/vfs.h>
#include <kint.h>

/**
 * @struct file_descriptor_t
 * @brief Represents a file descriptor entry
 *
 * Maps a file descriptor number to an open file and contains descriptor-specific
 * flags and state information.
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

typedef open_file_t file_descriptor_t;

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
open_file_t* fdm_open_file(vfs_entry_t* current);

/**
 * @brief Sets a file descriptor in a file_descriptor_entry_t
 * @param entry top level of file descriptor table
 * @param index fd element to set
 * @param fd file descriptor
 * @return returns 0 when completed successfully and -1 when failed
 *
 * Traverses through fd entries and returns the target file descriptor
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