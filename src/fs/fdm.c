#include <fs/fdm.h>

#include <fs/vfs.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>

open_file_t* fdm_open_file(vfs_entry_t* current)
{
    open_file_t* open_file = kmalloc(sizeof(open_file_t));
    open_file->offset = 0; // TODO: Set the offset
    open_file->refcount = 0;
    open_file->ops = current->ops;
    open_file->inode = current->inode;
    open_file->inode_num = current->inode_num;

    ext2_file_open(FILESYSTEM, open_file);
    // TODO: Set write, read, open, close to ops
    return open_file;
}