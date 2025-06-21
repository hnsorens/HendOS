#ifndef VFS_H
#define VFS_H

#include <drivers/ext2.h>

typedef struct list_head_t
{
    struct list_head_t* next;
    struct list_head_t* prev;
} list_head_t;

typedef struct vfs_entry_t
{
    uint32_t inode_num;
    ext2_inode* inode;
    char* name;
    struct vfs_entry_t* parent;
    list_head_t children;
    list_head_t sibling;
    entry_type_t type;
    uint32_t name_hash;
    uint8_t children_loaded;
} vfs_entry_t;

#define container_of(ptr, type, member) ((type*)((char*)(ptr)-offsetof(type, member)))

int vfs_find_entry(vfs_entry_t* current, vfs_entry_t** out, const char* path);
vfs_entry_t* vfs_create_entry(vfs_entry_t* dir, const char* name, entry_type_t type);
void vfs_open_file(vfs_entry_t* entry);
void vfs_init();

#endif