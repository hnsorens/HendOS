/**
 * @file file_descriptor_manager.c
 * @brief File Descriptor Management Implementation
 *
 * Implements core functionality for managing open files and file descriptors,
 * including file opening operations and descriptor management.
 */

#include <fs/fdm.h>
#include <fs/vfs.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>

/**
 * @brief Opens a file and initializes its open file structure
 * @param current VFS entry representing the file to open
 * @return Pointer to initialized file_descriptor_t structure
 *
 * This function:
 * 1. Allocates memory for a new open file structure
 * 2. Initializes all fields with default values
 * 3. Sets up the file operations from the VFS entry
 * 4. Opens the file through the filesystem driver
 */
file_descriptor_t* fdm_open_file(vfs_entry_t* current)
{
    /* Allocate memory for the open file structure */
    file_descriptor_t* open_file = pool_allocate(*OPEN_FILE_POOL);

    /* Initialize basic file properties */
    open_file->offset = 0;
    open_file->refcount = 0;
    open_file->ops = current->ops;
    open_file->inode = current->inode;
    open_file->inode_num = current->inode_num;
    open_file->type = current->type;
    open_file->private_data = current->private_data;

    /* Call filesystem-specific open operation */
    ext2_file_open(FILESYSTEM, open_file);

    /* TODO: Implement proper operation setup */
    /* TODO: Set write, read, open, close to ops */

    return open_file;
}

/**
 * @brief Sets a file descriptor in a file_descriptor_entry_t
 * @param entry top level of file descriptor table
 * @param index fd element to set
 * @param fd file descriptor
 * @return returns 0 when completed successfully and -1 when failed
 *
 * Traverses through fd entries and returns the target file descriptor
 */
int fdm_set(file_descriptor_entry_t* entry, size_t index, file_descriptor_t* fd)
{
    /* Returns -1 entry isnt valid of if index is too high */
    if (!entry || index > FD_ENTRY_COUNT * FD_ENTRY_COUNT)
        return -1;

    /* Gets indices */
    size_t first_index = index / FD_ENTRY_COUNT;
    size_t second_index = index % FD_ENTRY_COUNT;

    /* Allocates new page if one isnt there */
    if (!entry->file_descriptors[first_index])
    {
        entry->file_descriptors[first_index] = 0; // pool_allocate(*FD_ENTRY_POOL);
    }

    /* Sets file descriptor */
    ((file_descriptor_t***)entry->file_descriptors)[first_index][second_index] = fd;
    return 0;
}

/**
 * @brief Gets a file descriptor in a file_descriptor_entry_t
 * @param entry top level of file descriptor table
 * @param index fd element to set
 * @return Requested file descriptor or null if failed
 */
file_descriptor_t* fdm_get(file_descriptor_entry_t* entry, size_t index)
{
    /* Returns -1 entry isnt valid of if index is too high */
    if (!entry || index > FD_ENTRY_COUNT * FD_ENTRY_COUNT)
        return -1;

    /* Gets indices */
    size_t first_index = index / FD_ENTRY_COUNT;
    size_t second_index = index % FD_ENTRY_COUNT;

    /* Makes sure entry exists */
    if (!entry->file_descriptors[first_index])
        return -1;

    return ((file_descriptor_t***)entry->file_descriptors)[first_index][second_index];
}

/**
 * @brief Copies the entries in a file descriptor table
 * @param src top of source file descriptor table
 * @param dst top of destination file descriptor table
 * @return 0 on success, -1 on failure
 */
int fdm_copy(file_descriptor_entry_t* src, file_descriptor_entry_t* dst)
{
    /* Copies top entries */
    for (int i = 0; i < FD_ENTRY_COUNT; ++i)
    {
        /* If entry exists, continue copying */
        if (src->file_descriptors[i])
        {
            /* Copy file descriptors */
            file_descriptor_entry_t* current = 0; // pool_allocate(*FD_ENTRY_POOL);
            dst->file_descriptors[i] = current;
            kmemcpy(current, src->file_descriptors[i], sizeof(file_descriptor_entry_t));
        }
    }
    return 0;
}

/**
 * @brief Frees an entire file descriptor table, used on process cleanup
 * @param entry top of file descriptor entry
 */
void fdm_free(file_descriptor_entry_t* entry)
{
    for (int i = 0; i < FD_ENTRY_COUNT; i++)
    {
        if (entry->file_descriptors[i])
        {
            pool_free(entry->file_descriptors[i]);
        }
    }
    pool_free(entry);
}