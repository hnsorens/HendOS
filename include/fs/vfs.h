/**
 * @typedef file_ops_t
 * @brief File operation function pointer type
 * @param arg1 First operation argument
 * @param arg2 Second operation argument
 * @return Operation-specific return value
 */

#ifndef VFS_H
#define VFS_H

#include <drivers/ext2.h>

/**
 * @typedef file_ops_t
 * @brief File operation function pointer type
 * @param arg1 First operation argument
 * @param arg2 Second operation argument
 * @return Operation-specific return value
 */
typedef uint64_t (*file_ops_t)(uint64_t asd, uint64_t arg1, uint64_t arg2);

/**
 * @struct list_head_t
 * @brief Doubly-linked list node structure
 */
typedef struct list_head_t
{
    struct list_head_t* next; ///< Pointer to next list node
    struct list_head_t* prev; ///< Pointer to previous list node
} list_head_t;

/**
 * @struct vfs_entry_t
 * @brief Virtual filesystem entry structure
 *
 * Represents a filesystem object (file, directory, device) in the VFS layer.
 */
typedef struct vfs_entry_t
{
    uint32_t inode_num;         ///< Filesystem-specific inode number
    ext2_inode* inode;          ///< Pointer to underlying inode structure
    char* name;                 ///< Entry name
    struct vfs_entry_t* parent; ///< Pointer to parent directory
    list_head_t children;       ///< List of child entries
    list_head_t sibling;        ///< Sibling entries in parent directory
    entry_type_t type;          ///< Entry type (file, dir, etc.)
    uint32_t name_hash;         ///< Precomputed name hash
    uint8_t children_loaded;    ///< Flag indicating children were loaded
    file_ops_t* ops;            ///< File operations table
} vfs_entry_t;

/**
 * @brief Get container structure from member pointer
 * @param ptr Pointer to member
 * @param type Type of container structure
 * @param member Name of member in container
 * @return Pointer to container structure
 */
#define container_of(ptr, type, member) ((type*)((char*)(ptr)-offsetof(type, member)))

/* ==================== VFS Interface Functions ==================== */

/**
 * @brief Find VFS entry by path
 * @param current Starting directory entry
 * @param out Output parameter for found entry
 * @param path Path to resolve
 * @return 0 on success, non-zero on failure
 */
int vfs_find_entry(vfs_entry_t* current, vfs_entry_t** out, const char* path);

/**
 * @brief Create new VFS entry
 * @param dir Parent directory entry
 * @param name Name of new entry
 * @param type Type of new entry
 * @return Pointer to created entry
 */
vfs_entry_t* vfs_create_entry(vfs_entry_t* dir, const char* name, entry_type_t type);

/**
 * @brief Open file represented by VFS entry
 * @param entry VFS entry to open
 * @return Pointer to open file structure
 */
open_file_t* vfs_open_file(vfs_entry_t* entry);

/**
 * @brief Initialize VFS subsystem
 */
void vfs_init();

/**
 * @brief Get full path for VFS entry
 * @param dir Entry to get path for
 * @param buffer Output buffer for path
 * @param offset Pointer to current buffer offset
 */
void vfs_path(vfs_entry_t* dir, char* buffer, uint64_t* offset);

#endif /* VFS_H */