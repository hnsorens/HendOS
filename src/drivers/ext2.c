/* ext2.c */
#include <drivers/ext2.h>
// #include <string.h>
// #include <stdlib.h>
// #include <stdio.h>
// #include <time.h>
// #include <fs/fdm.h>
// #include <fs/vfs.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <misc/debug.h>

uint32_t time(uint32_t idk)
{
    return 0;
}

int strlen(const char* str)
{
    int len = 0;
    while (str[len])
        len++;
    return len;
}

int strlen16(const uint16_t* str)
{
    int len = 0;
    while (str[len])
        len++;
    return len;
}

char* strcpy(char* dest, const char* src)
{
    int i = 0;
    while ((dest[i] = src[i]))
        i++;
    return dest;
}

char* strcpy16(uint16_t* dest, const uint16_t* src)
{
    int i = 0;
    while ((dest[i] = src[i]))
        i++;
    return dest;
}

char* strncpy(char* dest, const char* src, int n)
{
    int i = 0;
    while (i < n && src[i])
    {
        dest[i] = src[i];
        i++;
    }
    while (i < n)
        dest[i++] = '\0';
    return dest;
}

int strcmp(const char* s1, const char* s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strcmp_8_16(const char* s1, const uint16_t* s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strcmp_16_8(const uint16_t* s1, const char* s2)
{
    while ((char)*s1 && ((char)*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, int n)
{
    while (n-- > 0 && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return n < 0 ? 0 : *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* strcat(char* dest, const char* src)
{
    int i = 0, j = 0;
    while (dest[i])
        i++;
    while ((dest[i++] = src[j++]))
        ;
    return dest;
}

char* strncat(char* dest, const char* src, int n)
{
    int i = 0, j = 0;
    while (dest[i])
        i++;
    while (j < n && src[j])
    {
        dest[i++] = src[j++];
    }
    dest[i] = '\0';
    return dest;
}

char* strchr(const char* str, char c)
{
    while (*str)
    {
        if (*str == c)
            return (char*)str;
        str++;
    }
    return c == '\0' ? (char*)str : 0;
}

char* strrchr(const char* str, char c)
{
    const char* last = 0;
    while (*str)
    {
        if (*str == c)
            last = str;
        str++;
    }
    return c == '\0' ? (char*)str : (char*)last;
}

char* strstr(const char* haystack, const char* needle)
{
    if (!*needle)
        return (char*)haystack;
    for (; *haystack; haystack++)
    {
        const char *h = haystack, *n = needle;
        while (*h && *n && (*h == *n))
        {
            h++;
            n++;
        }
        if (!*n)
            return (char*)haystack;
    }
    return 0;
}

// Internal functions
int read_inode(ext2_fs_t* fs, uint32_t inode_num, struct ext2_inode* inode);
static int write_inode(ext2_fs_t* fs, uint32_t inode_num, struct ext2_inode* inode);
static void* read_block(ext2_fs_t* fs, uint32_t block_num);
static int write_block(ext2_fs_t* fs, uint32_t block_num, void* data);
static uint32_t allocate_block(ext2_fs_t* fs);
static int free_block(ext2_fs_t* fs, uint32_t block_num);
static uint32_t allocate_inode(ext2_fs_t* fs, int is_directory);
static int free_inode(ext2_fs_t* fs, uint32_t inode_num);
static int add_entry(ext2_fs_t* fs,
                     uint32_t dir_inode,
                     const char* name,
                     uint32_t inode_num,
                     uint8_t file_type);
static int remove_entry(ext2_fs_t* fs, uint32_t dir_inode, const char* name);
static int find_entry(ext2_fs_t* fs,
                      uint32_t dir_inode,
                      const char* name,
                      uint32_t* inode_out,
                      uint8_t* file_type);
static int read_block_pointers(ext2_fs_t* fs,
                               struct ext2_inode* inode,
                               uint32_t block_idx,
                               uint32_t* blocks,
                               uint32_t count);
static int write_block_pointers(ext2_fs_t* fs,
                                struct ext2_inode* inode,
                                uint32_t block_idx,
                                uint32_t* blocks,
                                uint32_t count);
static int
update_file_size(ext2_fs_t* fs, uint32_t inode_num, struct ext2_inode* inode, uint32_t new_size);
static uint32_t count_blocks_needed(ext2_fs_t* fs, uint32_t size);
static int
ensure_blocks_allocated(ext2_fs_t* fs, struct ext2_inode* inode, uint32_t required_blocks);

// Initialize filesystem
int ext2_init(ext2_fs_t* fs,
              void* (*read_fn)(uint32_t, uint32_t),
              void (*write_fn)(uint32_t, uint32_t, void*),
              uint32_t start,
              uint32_t end)
{
    fs->read_sectors = read_fn;
    fs->write_sectors = write_fn;
    fs->start_sector = start;
    fs->end_sector = end;
    fs->block_buffer = kmalloc(SECTOR_SIZE * 2);

    // Read superblock (at offset 1024)
    void* superblock_sector = fs->read_sectors(fs->start_sector + 2, 2);
    struct ext2_superblock* sb =
        (struct ext2_superblock*)((uint8_t*)superblock_sector + 1024 % SECTOR_SIZE);

    if (sb->magic != EXT2_SIGNATURE)
    {
        kfree(fs->block_buffer);
        kfree(superblock_sector);
        return -1;
    }

    fs->block_size = 1024 << sb->log_block_size;
    fs->blocks_per_group = sb->blocks_per_group;
    fs->inodes_per_group = sb->inodes_per_group;
    fs->first_data_block = sb->first_data_block;
    fs->total_blocks = sb->blocks_count;
    fs->total_inodes = sb->inodes_count;
    fs->groups_count = (sb->blocks_count + sb->blocks_per_group - 1) / sb->blocks_per_group;
    fs->bgdt_block = (sb->first_data_block == 0) ? 1 : sb->first_data_block + 1;
    fs->inode_size = (sb->rev_level >= 1) ? sb->inode_size : 128;

    kfree(superblock_sector);
    return 0;
}

void ext2_cleanup(ext2_fs_t* fs)
{
    kfree(fs->block_buffer);
}

// File operations
int ext2_file_open(ext2_fs_t* fs, open_file_t* entry)
{
    if (read_inode(fs, entry->inode_num, entry->inode) != 0)
    {
        return -1;
    }

    if (!(entry->inode->mode & EXT2_S_IFREG))
    { // Not a regular file
        return -1;
    }
    return 0;
}

int ext2_file_create(ext2_fs_t* fs, uint32_t dir_inode, const char* filename, uint16_t mode)
{
    // Check if file already exists
    uint32_t existing_inode;
    if (find_entry(fs, dir_inode, filename, &existing_inode, NULL) == 0)
    {
        return -1; // File exists
    }

    // Allocate new inode
    uint32_t new_inode = allocate_inode(fs, 0);
    if (new_inode == 0)
    {
        return -1;
    }

    // Initialize inode
    struct ext2_inode inode = {0};
    inode.mode = EXT2_S_IFREG | (mode & 0x0FFF); // Regular file
    inode.uid = 0;
    inode.gid = 0;
    inode.size = 0;
    inode.atime = time(NULL);
    inode.ctime = time(NULL);
    inode.mtime = time(NULL);
    inode.dtime = 0;
    inode.links_count = 1;

    if (write_inode(fs, new_inode, &inode) != 0)
    {
        free_inode(fs, new_inode);
        return -1;
    }

    // Add directory entry
    if (add_entry(fs, dir_inode, filename, new_inode, EXT2_FT_REG_FILE) != 0)
    {
        free_inode(fs, new_inode);
        return -1;
    }

    return 0;
}

int ext2_file_close(ext2_fs_t* fs, open_file_t* file)
{
    // Update access time
    file->inode->atime = time(NULL);
    write_inode(fs, file->inode_num, file->inode);
    return 0;
}

long ext2_file_read2(ext2_fs_t* fs, open_file_t* file, void* buf, size_t count)
{
    if (file->pos >= file->inode->size)
    {
        return 0;
    }

    if (file->pos + count > file->inode->size)
    {
        count = file->inode->size - file->pos;
    }
    // while(1);
    size_t bytes_read = 0;
    uint32_t block_size = fs->block_size;

    while (count > 0)
    {
        uint32_t block_idx = file->pos / block_size;
        uint32_t block_offset = file->pos % block_size;
        size_t to_read = (count < block_size - block_offset) ? count : block_size - block_offset;

        uint32_t block_num;
        if (read_block_pointers(fs, file->inode, block_idx, &block_num, 1) != 1)
        {
            break;
        }

        if (block_num == 0)
        { // Sparse block
            kmemset((uint8_t*)buf + bytes_read, 0, to_read);
        }
        else
        {
            void* block_data = read_block(fs, block_num);
            kmemcpy((uint8_t*)buf + bytes_read, (uint8_t*)block_data + block_offset, to_read);
            kfree(block_data);
        }

        bytes_read += to_read;
        file->pos += to_read;
        count -= to_read;
    }

    // Update access time
    file->inode->atime = time(NULL);
    write_inode(fs, file->inode_num, file->inode);

    return bytes_read;
}

long ext2_file_read(ext2_fs_t* fs, open_file_t* file, void* buf, size_t count)
{
    if (file->pos >= file->inode->size)
    {
        return 0;
    }

    if (file->pos + count > file->inode->size)
    {
        count = file->inode->size - file->pos;
    }

    size_t bytes_read = 0;
    uint32_t block_size = fs->block_size;

    while (count > 0)
    {
        uint32_t block_idx = file->pos / block_size;
        uint32_t block_offset = file->pos % block_size;
        size_t to_read = (count < block_size - block_offset) ? count : block_size - block_offset;

        uint32_t block_num;
        if (read_block_pointers(fs, file->inode, block_idx, &block_num, 1) != 1)
        {
            break;
        }

        if (block_num == 0)
        { // Sparse block
            kmemset((uint8_t*)buf + bytes_read, 0, to_read);
        }
        else
        {
            void* block_data = read_block(fs, block_num);
            kmemcpy((uint8_t*)buf + bytes_read, (uint8_t*)block_data + block_offset, to_read);
            kfree(block_data);
        }

        bytes_read += to_read;
        file->pos += to_read;
        count -= to_read;
    }

    // Update access time
    file->inode->atime = time(NULL);
    write_inode(fs, file->inode_num, file->inode);
    return bytes_read;
}

long ext2_file_write(ext2_fs_t* fs, open_file_t* file, const void* buf, size_t count)
{
    size_t bytes_written = 0;
    uint32_t block_size = fs->block_size;

    // Calculate how many blocks we'll need
    uint32_t required_blocks = count_blocks_needed(fs, file->pos + count);
    uint32_t current_blocks = count_blocks_needed(fs, file->inode->size);

    if (required_blocks > current_blocks)
    {
        if (ensure_blocks_allocated(fs, file->inode, required_blocks) != 0)
        {
            return -1;
        }
    }

    while (count > 0)
    {
        uint32_t block_idx = file->pos / block_size;
        uint32_t block_offset = file->pos % block_size;
        size_t to_write = (count < block_size - block_offset) ? count : block_size - block_offset;

        uint32_t block_num;
        if (read_block_pointers(fs, file->inode, block_idx, &block_num, 1) != 1)
        {
            break;
        }

        if (block_num == 0)
        {
            // This shouldn't happen since we pre-allocated blocks
            break;
        }

        void* block_data = read_block(fs, block_num);
        kmemcpy((uint8_t*)block_data + block_offset, (uint8_t*)buf + bytes_written, to_write);
        write_block(fs, block_num, block_data);
        kfree(block_data);

        bytes_written += to_write;
        file->pos += to_write;
        count -= to_write;
    }

    // Update size if we extended the file
    if (file->pos > file->inode->size)
    {
        file->inode->size = file->pos;
        file->inode->mtime = time(NULL);
        file->inode->ctime = time(NULL);
        write_inode(fs, file->inode_num, file->inode);
    }

    return bytes_written;
}

int ext2_file_seek(open_file_t* file, long offset, int whence)
{
    size_t new_pos;

    switch (whence)
    {
    case SEEK_SET:
        new_pos = offset;
        break;
    case SEEK_CUR:
        new_pos = file->pos + offset;
        break;
    case SEEK_END:
        new_pos = file->inode->size + offset;
        break;
    default:
        return -1;
    }

    if (new_pos > file->inode->size)
    {
        return -1;
    }

    file->pos = new_pos;
    return 0;
}

int ext2_file_truncate(ext2_fs_t* fs, open_file_t* file, size_t length)
{
    if (length == file->inode->size)
    {
        return 0;
    }

    if (length > file->inode->size)
    {
        // Extend file - allocate new blocks
        uint32_t required_blocks = count_blocks_needed(fs, length);
        uint32_t current_blocks = count_blocks_needed(fs, file->inode->size);

        if (required_blocks > current_blocks)
        {
            if (ensure_blocks_allocated(fs, file->inode, required_blocks) != 0)
            {
                return -1;
            }
        }

        file->inode->size = length;
    }
    else
    {
        // Shrink file - free blocks
        uint32_t old_blocks = count_blocks_needed(fs, file->inode->size);
        uint32_t new_blocks = count_blocks_needed(fs, length);

        if (new_blocks < old_blocks)
        {
            uint32_t blocks_to_free = old_blocks - new_blocks;
            uint32_t* blocks = kmalloc(blocks_to_free * sizeof(uint32_t));

            if (read_block_pointers(fs, file->inode, new_blocks, blocks, blocks_to_free) !=
                blocks_to_free)
            {
                kfree(blocks);
                return -1;
            }

            for (uint32_t i = 0; i < blocks_to_free; i++)
            {
                if (blocks[i] != 0)
                {
                    free_block(fs, blocks[i]);
                }
            }

            kfree(blocks);

            // Zero out the pointers
            uint32_t zero = 0;
            write_block_pointers(fs, file->inode, new_blocks, &zero, blocks_to_free);
        }

        file->inode->size = length;
    }

    file->inode->mtime = time(NULL);
    file->inode->ctime = time(NULL);

    if (write_inode(fs, file->inode_num, file->inode) != 0)
    {
        return -1;
    }

    if (file->pos > length)
    {
        file->pos = length;
    }

    return 0;
}

int ext2_file_delete(ext2_fs_t* fs, uint32_t dir_inode, const char* filename)
{
    // Find the file
    uint32_t file_inode;
    uint8_t file_type;
    if (find_entry(fs, dir_inode, filename, &file_inode, &file_type) != 0)
    {
        return -1;
    }

    if (file_type != EXT2_FT_REG_FILE)
    {
        return -1;
    }

    // Remove directory entry
    if (remove_entry(fs, dir_inode, filename) != 0)
    {
        return -1;
    }

    // Get file inode
    struct ext2_inode inode;
    if (read_inode(fs, file_inode, &inode) != 0)
    {
        return -1;
    }

    // Free all blocks
    uint32_t blocks_count = count_blocks_needed(fs, inode.size);
    uint32_t* blocks = kmalloc(blocks_count * sizeof(uint32_t));

    if (read_block_pointers(fs, &inode, 0, blocks, blocks_count) != blocks_count)
    {
        kfree(blocks);
        return -1;
    }

    for (uint32_t i = 0; i < blocks_count; i++)
    {
        if (blocks[i] != 0)
        {
            free_block(fs, blocks[i]);
        }
    }

    kfree(blocks);

    // Free inode
    if (free_inode(fs, file_inode) != 0)
    {
        return -1;
    }

    return 0;
}

// Directory operations
int ext2_dir_create(ext2_fs_t* fs, uint32_t parent_inode, const char* dirname, uint16_t mode)
{
    // Check if directory already exists
    uint32_t existing_inode;
    if (find_entry(fs, parent_inode, dirname, &existing_inode, NULL) == 0)
    {
        return -1;
    }

    // Allocate new inode
    uint32_t new_inode = allocate_inode(fs, 1);
    if (new_inode == 0)
    {
        return -1;
    }

    // Initialize inode
    struct ext2_inode inode = {0};
    inode.mode = EXT2_S_IFDIR | (mode & 0x0FFF); // Directory
    inode.uid = 0;
    inode.gid = 0;
    inode.size = fs->block_size;
    inode.atime = time(NULL);
    inode.ctime = time(NULL);
    inode.mtime = time(NULL);
    inode.dtime = 0;
    inode.links_count = 2; // '.' and parent link

    // Allocate first block for directory
    uint32_t block_num = allocate_block(fs);
    if (block_num == 0)
    {
        free_inode(fs, new_inode);
        return -1;
    }

    inode.block[0] = block_num;

    if (write_inode(fs, new_inode, &inode) != 0)
    {
        free_block(fs, block_num);
        free_inode(fs, new_inode);
        return -1;
    }

    // Add entry to parent directory
    if (add_entry(fs, parent_inode, dirname, new_inode, EXT2_FT_DIR) != 0)
    {
        // Clean up
        free_block(fs, block_num);
        free_inode(fs, new_inode);
        return -1;
    }

    // Update parent directory link count
    struct ext2_inode parent_inode_data;
    if (read_inode(fs, parent_inode, &parent_inode_data) == 0)
    {
        parent_inode_data.links_count++;
        parent_inode_data.mtime = time(NULL);
        parent_inode_data.ctime = time(NULL);
        write_inode(fs, parent_inode, &parent_inode_data);
    }

    return 0;
}

int ext2_dir_delete(ext2_fs_t* fs, uint32_t parent_inode, const char* dirname)
{
    // Find the directory
    uint32_t dir_inode;
    uint8_t file_type;
    if (find_entry(fs, parent_inode, dirname, &dir_inode, &file_type) != 0)
    {
        return -1;
    }

    if (file_type != EXT2_FT_DIR)
    {
        return -1;
    }

    // Check if directory is empty
    ext2_dirent_iter_t iter;
    ext2_dirent_t* dirent;

    if (ext2_dir_iter_start(fs, &iter, dir_inode) == 0)
    {
        int count = 0;
        while (ext2_dir_iter_next(fs, &iter, &dirent) == 0)
        {
            count++;
            break;
        }
        ext2_dir_iter_end(&iter);

        if (count > 0)
        {
            return -1; // Directory not empty
        }
    }

    // Remove directory entry from parent
    if (remove_entry(fs, parent_inode, dirname) != 0)
    {
        return -1;
    }

    // Get directory inode
    struct ext2_inode inode;
    if (read_inode(fs, dir_inode, &inode) != 0)
    {
        return -1;
    }

    // Free all blocks
    uint32_t blocks_count = count_blocks_needed(fs, inode.size);
    uint32_t* blocks = kmalloc(blocks_count * sizeof(uint32_t));

    if (read_block_pointers(fs, &inode, 0, blocks, blocks_count) != blocks_count)
    {
        kfree(blocks);
        return -1;
    }

    for (uint32_t i = 0; i < blocks_count; i++)
    {
        if (blocks[i] != 0)
        {
            free_block(fs, blocks[i]);
        }
    }

    kfree(blocks);

    // Free inode
    if (free_inode(fs, dir_inode) != 0)
    {
        return -1;
    }

    // Update parent directory link count
    struct ext2_inode parent_inode_data;
    if (read_inode(fs, parent_inode, &parent_inode_data) == 0)
    {
        parent_inode_data.links_count--;
        parent_inode_data.mtime = time(NULL);
        parent_inode_data.ctime = time(NULL);
        write_inode(fs, parent_inode, &parent_inode_data);
    }

    return 0;
}

int ext2_dir_count_entries(ext2_fs_t* fs, uint32_t dir_inode)
{
    struct ext2_inode inode;
    if (read_inode(fs, dir_inode, &inode) != 0)
    {
        return -1;
    }

    if (!(inode.mode & EXT2_S_IFDIR))
    {
        return -1; // Not a directory
    }

    int count = 0;
    uint8_t* buffer = kmalloc(fs->block_size);
    uint32_t offset = 0;

    while (offset < inode.size)
    {
        uint32_t block_idx = offset / fs->block_size;
        uint32_t block_num;

        if (read_block_pointers(fs, &inode, block_idx, &block_num, 1) != 1)
        {
            kfree(buffer);
            return -1;
        }

        if (block_num == 0)
        {
            kfree(buffer);
            return -1;
        }

        void* block_data = read_block(fs, block_num);
        uint32_t blockSize = fs->block_size;
        kmemcpy(buffer, block_data, fs->block_size);
        kfree(block_data);

        uint32_t pos = 0;
        while (pos < fs->block_size)
        {
            ext2_dirent_t* entry = (ext2_dirent_t*)(buffer + pos);
            if (entry->rec_len == 0)
            {
                break;
            }

            if (entry->inode != 0)
            { // Count only valid entries
                count++;
            }

            pos += entry->rec_len;
        }

        offset += fs->block_size;
    }

    kfree(buffer);
    return count;
}

int ext2_dir_iter_start(ext2_fs_t* fs, ext2_dirent_iter_t* iter, uint32_t inode_num)
{
    ext2_inode inode;
    if (read_inode(fs, inode_num, &inode) != 0)
    {
        return -1;
    }

    if (!(inode.mode & EXT2_S_IFDIR))
    { // Not a directory
        return -1;
    }

    iter->buffer = kmalloc(fs->block_size);
    iter->pos = 0;
    iter->block_remaining = 0;
    iter->current_block = 0;
    iter->blocks_remaining = count_blocks_needed(fs, inode.size);
    iter->inode = inode_num;

    return 0;
}

int ext2_dir_iter_next(ext2_fs_t* fs, ext2_dirent_iter_t* iter, ext2_dirent_t** dirent)
{
    while (1)
    {
        if (iter->pos >= iter->block_remaining)
        {
            if (iter->blocks_remaining == 0)
            {
                return -1; // No more blocks
            }

            uint32_t block_num;
            struct ext2_inode inode;
            if (read_inode(fs, iter->inode, &inode) != 0)
            {
                return -1;
            }

            if (read_block_pointers(fs, &inode, iter->current_block, &block_num, 1) != 1)
            {
                return -1;
            }

            if (block_num == 0)
            { // Sparse block
                kmemset(iter->buffer, 0, fs->block_size);
            }
            else
            {
                void* block_data = read_block(fs, block_num);
                kmemcpy(iter->buffer, block_data, fs->block_size);
                kfree(block_data);
            }

            iter->pos = 0;
            iter->block_remaining = fs->block_size;
            iter->blocks_remaining--;
            iter->current_block++;
        }

        ext2_dirent_t* entry = (ext2_dirent_t*)((uint8_t*)iter->buffer + iter->pos);
        if (entry->rec_len == 0)
        {
            iter->pos = iter->block_remaining;
            continue;
        }

        if (entry->inode == 0)
        {
            iter->pos += entry->rec_len;
            continue;
        }

        *dirent = entry;
        iter->pos += entry->rec_len;
        return 0;
    }
}

void ext2_dir_iter_end(ext2_dirent_iter_t* iter)
{
    kfree(iter->buffer);
}

int ext2_stat(ext2_fs_t* fs, uint32_t inode_num, ext2_inode* inode_out)
{
    return read_inode(fs, inode_num, inode_out);
}

int ext2_rename(ext2_fs_t* fs,
                uint32_t old_dir_inode,
                uint32_t new_dir_inode,
                const char* old_filename,
                const char* new_filename)
{
    // Find the file in old directory
    uint32_t file_inode;
    uint8_t file_type;
    if (find_entry(fs, old_dir_inode, old_filename, &file_inode, &file_type) != 0)
    {
        return -1;
    }

    // Check if target already exists
    uint32_t existing_inode;
    if (find_entry(fs, new_dir_inode, new_filename, &existing_inode, NULL) == 0)
    {
        return -1;
    }

    // Remove entry from old directory
    if (remove_entry(fs, old_dir_inode, old_filename) != 0)
    {
        return -1;
    }

    // Add entry to new directory
    if (add_entry(fs, new_dir_inode, new_filename, file_inode, file_type) != 0)
    {
        // Try to restore old entry
        add_entry(fs, old_dir_inode, old_filename, file_inode, file_type);
        return -1;
    }

    return 0;
}

int ext2_get_size(ext2_fs_t* fs, const char* path, uint32_t* size_out)
{
    struct ext2_inode inode;
    if (ext2_stat(fs, path, &inode) != 0)
    {
        return -1;
    }

    *size_out = inode.size;
    return 0;
}

int ext2_is_dir(ext2_fs_t* fs, const char* path)
{
    struct ext2_inode inode;
    if (ext2_stat(fs, path, &inode) != 0)
    {
        return 0;
    }

    return (inode.mode & EXT2_S_IFDIR) ? 1 : 0;
}

int ext2_is_file(ext2_fs_t* fs, const char* path)
{
    struct ext2_inode inode;
    if (ext2_stat(fs, path, &inode) != 0)
    {
        return 0;
    }

    return (inode.mode & EXT2_S_IFREG) ? 1 : 0;
}

// Internal helper functions
int read_inode(ext2_fs_t* fs, uint32_t inode_num, struct ext2_inode* inode)
{
    if (inode_num < 1 || inode_num > fs->total_inodes)
    {
        return -1;
    }

    uint32_t group = (inode_num - 1) / fs->inodes_per_group;
    uint32_t index = (inode_num - 1) % fs->inodes_per_group;

    struct ext2_bg_desc bg_desc;
    void* bgdt_data =
        read_block(fs, fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)));
    kmemcpy(&bg_desc,
            (uint8_t*)bgdt_data + (group % (fs->block_size / sizeof(struct ext2_bg_desc))) *
                                      sizeof(struct ext2_bg_desc),
            sizeof(struct ext2_bg_desc));
    kfree(bgdt_data);

    uint32_t inode_table_block = bg_desc.inode_table;
    uint32_t inode_offset = index * fs->inode_size;
    uint32_t inode_block = inode_table_block + (inode_offset / fs->block_size);
    uint32_t inode_block_offset = inode_offset % fs->block_size;

    void* block = read_block(fs, inode_block);
    kmemcpy(inode, (uint8_t*)block + inode_block_offset, sizeof(struct ext2_inode));
    kfree(block);

    return 0;
}

static int write_inode(ext2_fs_t* fs, uint32_t inode_num, struct ext2_inode* inode)
{
    if (inode_num < 1 || inode_num > fs->total_inodes)
    {
        return -1;
    }

    uint32_t group = (inode_num - 1) / fs->inodes_per_group;
    uint32_t index = (inode_num - 1) % fs->inodes_per_group;

    struct ext2_bg_desc bg_desc;
    void* bgdt_data =
        read_block(fs, fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)));
    kmemcpy(&bg_desc,
            (uint8_t*)bgdt_data + (group % (fs->block_size / sizeof(struct ext2_bg_desc))) *
                                      sizeof(struct ext2_bg_desc),
            sizeof(struct ext2_bg_desc));
    kfree(bgdt_data);

    uint32_t inode_table_block = bg_desc.inode_table;
    uint32_t inode_offset = index * fs->inode_size;
    uint32_t inode_block = inode_table_block + (inode_offset / fs->block_size);
    uint32_t inode_block_offset = inode_offset % fs->block_size;

    void* block = read_block(fs, inode_block);
    kmemcpy((uint8_t*)block + inode_block_offset, inode, sizeof(struct ext2_inode));
    write_block(fs, inode_block, block);
    kfree(block);

    return 0;
}

static void* read_block(ext2_fs_t* fs, uint32_t block_num)
{
    if (block_num >= fs->total_blocks)
    {
        return NULL;
    }

    return fs->read_sectors(fs->start_sector + block_num * (fs->block_size / SECTOR_SIZE),
                            fs->block_size / SECTOR_SIZE);
}

static int write_block(ext2_fs_t* fs, uint32_t block_num, void* data)
{
    if (block_num >= fs->total_blocks)
    {
        return -1;
    }

    fs->write_sectors(fs->start_sector + block_num * (fs->block_size / SECTOR_SIZE),
                      fs->block_size / SECTOR_SIZE, data);
    return 0;
}

static uint32_t allocate_block(ext2_fs_t* fs)
{
    // Read block bitmap for each group until we find a free block
    for (uint32_t group = 0; group < fs->groups_count; group++)
    {
        struct ext2_bg_desc bg_desc;
        void* bgdt_data =
            read_block(fs, fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)));
        kmemcpy(&bg_desc,
                (uint8_t*)bgdt_data + (group % (fs->block_size / sizeof(struct ext2_bg_desc))) *
                                          sizeof(struct ext2_bg_desc),
                sizeof(struct ext2_bg_desc));
        kfree(bgdt_data);

        if (bg_desc.free_blocks_count == 0)
        {
            continue;
        }

        // Read block bitmap
        void* bitmap_block = read_block(fs, bg_desc.block_bitmap);
        uint8_t* bitmap = (uint8_t*)bitmap_block;

        // Find first free block
        uint32_t blocks_in_group = (group == fs->groups_count - 1)
                                       ? (fs->total_blocks - group * fs->blocks_per_group)
                                       : fs->blocks_per_group;

        for (uint32_t i = 0; i < blocks_in_group; i++)
        {
            if ((bitmap[i / 8] & (1 << (i % 8))) == 0)
            {
                // Mark block as used
                bitmap[i / 8] |= (1 << (i % 8));
                write_block(fs, bg_desc.block_bitmap, bitmap);

                // Update block group descriptor
                bg_desc.free_blocks_count--;
                void* bgdt_block = read_block(
                    fs, fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)));
                kmemcpy((uint8_t*)bgdt_block +
                            (group % (fs->block_size / sizeof(struct ext2_bg_desc))) *
                                sizeof(struct ext2_bg_desc),
                        &bg_desc, sizeof(struct ext2_bg_desc));
                write_block(fs,
                            fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)),
                            bgdt_block);
                kfree(bgdt_block);

                // Update superblock
                void* superblock_sector = fs->read_sectors(fs->start_sector + 2, 2);
                struct ext2_superblock* sb =
                    (struct ext2_superblock*)((uint8_t*)superblock_sector + 1024 % SECTOR_SIZE);
                sb->free_blocks_count--;
                fs->write_sectors(fs->start_sector + 2, 2, superblock_sector);
                kfree(superblock_sector);

                kfree(bitmap_block);
                return group * fs->blocks_per_group + i + fs->first_data_block;
            }
        }

        kfree(bitmap_block);
    }

    return 0; // No free blocks
}

static int free_block(ext2_fs_t* fs, uint32_t block_num)
{
    if (block_num < fs->first_data_block || block_num >= fs->total_blocks)
    {
        return -1;
    }

    uint32_t group = (block_num - fs->first_data_block) / fs->blocks_per_group;
    uint32_t index = (block_num - fs->first_data_block) % fs->blocks_per_group;

    struct ext2_bg_desc bg_desc;
    void* bgdt_data =
        read_block(fs, fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)));
    kmemcpy(&bg_desc,
            (uint8_t*)bgdt_data + (group % (fs->block_size / sizeof(struct ext2_bg_desc))) *
                                      sizeof(struct ext2_bg_desc),
            sizeof(struct ext2_bg_desc));
    kfree(bgdt_data);

    // Read block bitmap
    void* bitmap_block = read_block(fs, bg_desc.block_bitmap);
    uint8_t* bitmap = (uint8_t*)bitmap_block;

    // Check if block is already free
    if ((bitmap[index / 8] & (1 << (index % 8))) == 0)
    {
        kfree(bitmap_block);
        return 0;
    }

    // Mark block as free
    bitmap[index / 8] &= ~(1 << (index % 8));
    write_block(fs, bg_desc.block_bitmap, bitmap);

    // Update block group descriptor
    bg_desc.free_blocks_count++;
    void* bgdt_block =
        read_block(fs, fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)));
    kmemcpy((uint8_t*)bgdt_block + (group % (fs->block_size / sizeof(struct ext2_bg_desc))) *
                                       sizeof(struct ext2_bg_desc),
            &bg_desc, sizeof(struct ext2_bg_desc));
    write_block(fs, fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)),
                bgdt_block);
    kfree(bgdt_block);

    // Update superblock
    void* superblock_sector = fs->read_sectors(fs->start_sector + 2, 2);
    struct ext2_superblock* sb =
        (struct ext2_superblock*)((uint8_t*)superblock_sector + 1024 % SECTOR_SIZE);
    sb->free_blocks_count++;
    fs->write_sectors(fs->start_sector + 2, 2, superblock_sector);
    kfree(superblock_sector);

    kfree(bitmap_block);
    return 0;
}

static uint32_t allocate_inode(ext2_fs_t* fs, int is_directory)
{
    // Read inode bitmap for each group until we find a free inode
    for (uint32_t group = 0; group < fs->groups_count; group++)
    {
        struct ext2_bg_desc bg_desc;
        void* bgdt_data =
            read_block(fs, fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)));
        kmemcpy(&bg_desc,
                (uint8_t*)bgdt_data + (group % (fs->block_size / sizeof(struct ext2_bg_desc))) *
                                          sizeof(struct ext2_bg_desc),
                sizeof(struct ext2_bg_desc));
        kfree(bgdt_data);

        if (bg_desc.free_inodes_count == 0)
        {
            continue;
        }

        // Read inode bitmap
        void* bitmap_block = read_block(fs, bg_desc.inode_bitmap);
        uint8_t* bitmap = (uint8_t*)bitmap_block;

        // Find first free inode (skip inode 0)
        for (uint32_t i = 0; i < fs->inodes_per_group; i++)
        {
            if (i == 0)
                continue; // Inode 0 is reserved

            if ((bitmap[i / 8] & (1 << (i % 8))) == 0)
            {
                // Mark inode as used
                bitmap[i / 8] |= (1 << (i % 8));
                write_block(fs, bg_desc.inode_bitmap, bitmap);

                // Update block group descriptor
                bg_desc.free_inodes_count--;
                if (is_directory)
                {
                    bg_desc.used_dirs_count++;
                }

                void* bgdt_block = read_block(
                    fs, fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)));
                kmemcpy((uint8_t*)bgdt_block +
                            (group % (fs->block_size / sizeof(struct ext2_bg_desc))) *
                                sizeof(struct ext2_bg_desc),
                        &bg_desc, sizeof(struct ext2_bg_desc));
                write_block(fs,
                            fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)),
                            bgdt_block);
                kfree(bgdt_block);

                // Update superblock
                void* superblock_sector = fs->read_sectors(fs->start_sector + 2, 2);
                struct ext2_superblock* sb =
                    (struct ext2_superblock*)((uint8_t*)superblock_sector + 1024 % SECTOR_SIZE);
                sb->free_inodes_count--;
                fs->write_sectors(fs->start_sector + 2, 2, superblock_sector);
                kfree(superblock_sector);

                kfree(bitmap_block);
                return group * fs->inodes_per_group + i + 1;
            }
        }

        kfree(bitmap_block);
    }

    return 0; // No free inodes
}

static int free_inode(ext2_fs_t* fs, uint32_t inode_num)
{
    if (inode_num < 1 || inode_num > fs->total_inodes)
    {
        return -1;
    }

    uint32_t group = (inode_num - 1) / fs->inodes_per_group;
    uint32_t index = (inode_num - 1) % fs->inodes_per_group;

    struct ext2_bg_desc bg_desc;
    void* bgdt_data =
        read_block(fs, fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)));
    kmemcpy(&bg_desc,
            (uint8_t*)bgdt_data + (group % (fs->block_size / sizeof(struct ext2_bg_desc))) *
                                      sizeof(struct ext2_bg_desc),
            sizeof(struct ext2_bg_desc));
    kfree(bgdt_data);

    // Read inode bitmap
    void* bitmap_block = read_block(fs, bg_desc.inode_bitmap);
    uint8_t* bitmap = (uint8_t*)bitmap_block;

    // Check if inode is already free
    if ((bitmap[index / 8] & (1 << (index % 8))) == 0)
    {
        kfree(bitmap_block);
        return 0;
    }

    // Mark inode as free
    bitmap[index / 8] &= ~(1 << (index % 8));
    write_block(fs, bg_desc.inode_bitmap, bitmap);

    // Update block group descriptor
    bg_desc.free_inodes_count++;
    void* bgdt_block =
        read_block(fs, fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)));
    kmemcpy((uint8_t*)bgdt_block + (group % (fs->block_size / sizeof(struct ext2_bg_desc))) *
                                       sizeof(struct ext2_bg_desc),
            &bg_desc, sizeof(struct ext2_bg_desc));
    write_block(fs, fs->bgdt_block + group / (fs->block_size / sizeof(struct ext2_bg_desc)),
                bgdt_block);
    kfree(bgdt_block);

    // Update superblock
    void* superblock_sector = fs->read_sectors(fs->start_sector + 2, 2);
    struct ext2_superblock* sb =
        (struct ext2_superblock*)((uint8_t*)superblock_sector + 1024 % SECTOR_SIZE);
    sb->free_inodes_count++;
    fs->write_sectors(fs->start_sector + 2, 2, superblock_sector);
    kfree(superblock_sector);

    kfree(bitmap_block);
    return 0;
}

static int find_entry(ext2_fs_t* fs,
                      uint32_t dir_inode,
                      const char* name,
                      uint32_t* inode_out,
                      uint8_t* file_type)
{
    struct ext2_inode inode;
    if (read_inode(fs, dir_inode, &inode) != 0)
    {
        return -1;
    }

    if (!(inode.mode & EXT2_S_IFDIR))
    { // Not a directory
        return -1;
    }

    uint32_t name_len = strlen(name);
    uint8_t* buffer = kmalloc(fs->block_size);
    uint32_t offset = 0;
    while (offset < inode.size)
    {
        uint32_t block_idx = offset / fs->block_size;
        uint32_t block_offset = offset % fs->block_size;

        uint32_t block_num;
        if (read_block_pointers(fs, &inode, block_idx, &block_num, 1) != 1)
        {
            kfree(buffer);
            return -1;
        }

        if (block_num == 0)
        {
            kfree(buffer);
            return -1;
        }

        void* block_data = read_block(fs, block_num);
        kmemcpy(buffer, block_data, fs->block_size);
        kfree(block_data);

        uint32_t pos = 0;
        while (pos < fs->block_size)
        {
            ext2_dirent_t* entry = (ext2_dirent_t*)(buffer + pos);
            if (entry->rec_len == 0)
            {
                break;
            }

            if (entry->inode != 0 && entry->name_len == name_len &&
                kmemcmp(entry->name, name, name_len) == 0)
            {
                *inode_out = entry->inode;
                if (file_type)
                {
                    *file_type = entry->file_type;
                }
                kfree(buffer);
                return 0;
            }

            pos += entry->rec_len;
        }

        offset += fs->block_size;
    }

    kfree(buffer);
    return -1;
}

static int add_entry(ext2_fs_t* fs,
                     uint32_t dir_inode,
                     const char* name,
                     uint32_t inode_num,
                     uint8_t file_type)
{
    struct ext2_inode inode;
    if (read_inode(fs, dir_inode, &inode) != 0)
    {
        return -1;
    }

    if (!(inode.mode & EXT2_S_IFDIR))
    { // Not a directory
        return -1;
    }

    uint32_t name_len = strlen(name);
    uint32_t entry_len = sizeof(ext2_dirent_t) + ((name_len + 3) & ~3); // Align to 4 bytes
    uint8_t* buffer = kmalloc(fs->block_size);
    uint32_t offset = 0;

    while (offset < inode.size)
    {
        uint32_t block_idx = offset / fs->block_size;
        uint32_t block_offset = offset % fs->block_size;

        uint32_t block_num;
        if (read_block_pointers(fs, &inode, block_idx, &block_num, 1) != 1)
        {
            kfree(buffer);
            return -1;
        }

        if (block_num == 0)
        {
            // Allocate new block for directory
            block_num = allocate_block(fs);
            if (block_num == 0)
            {
                kfree(buffer);
                return -1;
            }

            kmemset(buffer, 0, fs->block_size);
            write_block(fs, block_num, buffer);

            if (write_block_pointers(fs, &inode, block_idx, &block_num, 1) != 1)
            {
                free_block(fs, block_num);
                kfree(buffer);
                return -1;
            }

            inode.size += fs->block_size;
            write_inode(fs, dir_inode, &inode);
        }
        else
        {
            void* block_data = read_block(fs, block_num);
            kmemcpy(buffer, block_data, fs->block_size);
            kfree(block_data);
        }

        uint32_t pos = 0;
        while (pos < fs->block_size)
        {
            ext2_dirent_t* entry = (ext2_dirent_t*)(buffer + pos);

            if (entry->rec_len == 0)
            {
                // End of directory entries in this block
                if (fs->block_size - pos >= entry_len)
                {
                    // Create new entry at end
                    ext2_dirent_t* new_entry = (ext2_dirent_t*)(buffer + pos);
                    new_entry->inode = inode_num;
                    new_entry->rec_len = fs->block_size - pos;
                    new_entry->name_len = name_len;
                    new_entry->file_type = file_type;
                    kmemcpy(new_entry->name, name, name_len);

                    write_block(fs, block_num, buffer);
                    kfree(buffer);
                    return 0;
                }
                break;
            }

            if (entry->inode == 0)
            {
                // Unused entry - check if we can fit here
                if (entry->rec_len >= entry_len)
                {
                    // Create new entry in this space
                    ext2_dirent_t* new_entry = (ext2_dirent_t*)(buffer + pos);
                    new_entry->inode = inode_num;
                    new_entry->rec_len = entry->rec_len;
                    new_entry->name_len = name_len;
                    new_entry->file_type = file_type;
                    kmemcpy(new_entry->name, name, name_len);

                    // If there's remaining space, create a new unused entry
                    if (entry->rec_len > entry_len)
                    {
                        ext2_dirent_t* unused = (ext2_dirent_t*)(buffer + pos + entry_len);
                        unused->inode = 0;
                        unused->rec_len = entry->rec_len - entry_len;
                        unused->name_len = 0;
                    }

                    write_block(fs, block_num, buffer);
                    kfree(buffer);
                    return 0;
                }
            }
            else
            {
                // Check if we can split this entry
                uint32_t needed = entry_len;
                uint32_t available =
                    entry->rec_len - (sizeof(ext2_dirent_t) + ((entry->name_len + 3) & ~3));

                if (available >= needed)
                {
                    // Split the entry
                    uint32_t old_rec_len = entry->rec_len;
                    entry->rec_len = sizeof(ext2_dirent_t) + ((entry->name_len + 3) & ~3);

                    ext2_dirent_t* new_entry = (ext2_dirent_t*)(buffer + pos + entry->rec_len);
                    new_entry->inode = inode_num;
                    new_entry->rec_len = old_rec_len - entry->rec_len;
                    new_entry->name_len = name_len;
                    new_entry->file_type = file_type;
                    kmemcpy(new_entry->name, name, name_len);

                    write_block(fs, block_num, buffer);
                    kfree(buffer);
                    return 0;
                }
            }

            pos += entry->rec_len;
        }

        offset += fs->block_size;
    }

    // Need to add a new block to the directory
    uint32_t new_block = allocate_block(fs);
    if (new_block == 0)
    {
        kfree(buffer);
        return -1;
    }

    kmemset(buffer, 0, fs->block_size);
    ext2_dirent_t* new_entry = (ext2_dirent_t*)buffer;
    new_entry->inode = inode_num;
    new_entry->rec_len = fs->block_size;
    new_entry->name_len = name_len;
    new_entry->file_type = file_type;
    kmemcpy(new_entry->name, name, name_len);

    write_block(fs, new_block, buffer);

    // Add block to inode
    uint32_t block_idx = inode.size / fs->block_size;
    if (write_block_pointers(fs, &inode, block_idx, &new_block, 1) != 1)
    {
        free_block(fs, new_block);
        kfree(buffer);
        return -1;
    }

    inode.size += fs->block_size;
    write_inode(fs, dir_inode, &inode);

    kfree(buffer);
    return 0;
}

static int remove_entry(ext2_fs_t* fs, uint32_t dir_inode, const char* name)
{
    struct ext2_inode inode;
    if (read_inode(fs, dir_inode, &inode) != 0)
    {
        return -1;
    }

    if (!(inode.mode & EXT2_S_IFDIR))
    { // Not a directory
        return -1;
    }

    uint32_t name_len = strlen(name);
    uint8_t* buffer = kmalloc(fs->block_size);
    uint32_t offset = 0;
    ext2_dirent_t* prev_entry = NULL;
    uint32_t prev_pos = 0;

    while (offset < inode.size)
    {
        uint32_t block_idx = offset / fs->block_size;
        uint32_t block_offset = offset % fs->block_size;

        uint32_t block_num;
        if (read_block_pointers(fs, &inode, block_idx, &block_num, 1) != 1)
        {
            kfree(buffer);
            return -1;
        }

        if (block_num == 0)
        {
            kfree(buffer);
            return -1;
        }

        void* block_data = read_block(fs, block_num);
        kmemcpy(buffer, block_data, fs->block_size);
        kfree(block_data);

        uint32_t pos = 0;
        while (pos < fs->block_size)
        {
            ext2_dirent_t* entry = (ext2_dirent_t*)(buffer + pos);
            if (entry->rec_len == 0)
            {
                break;
            }

            if (entry->inode != 0 && entry->name_len == name_len &&
                kmemcmp(entry->name, name, name_len) == 0)
            {
                // Found the entry to remove
                entry->inode = 0;

                // Merge with previous entry if possible
                if (prev_entry != NULL)
                {
                    prev_entry->rec_len += entry->rec_len;
                }

                write_block(fs, block_num, buffer);
                kfree(buffer);
                return 0;
            }

            prev_entry = entry;
            prev_pos = pos;
            pos += entry->rec_len;
        }

        offset += fs->block_size;
    }

    kfree(buffer);
    return -1;
}

static int read_block_pointers(ext2_fs_t* fs,
                               struct ext2_inode* inode,
                               uint32_t block_idx,
                               uint32_t* blocks,
                               uint32_t count)
{
    uint32_t blocks_read = 0;
    uint32_t ptrs_per_block = fs->block_size / sizeof(uint32_t);

    // Direct blocks
    if (block_idx < 12)
    {
        uint32_t to_read = (count < 12 - block_idx) ? count : 12 - block_idx;
        kmemcpy(blocks, &inode->block[block_idx], to_read * sizeof(uint32_t));
        blocks_read += to_read;
        block_idx += to_read;
        count -= to_read;
    }

    // Single indirect block
    if (count > 0 && block_idx < 12 + ptrs_per_block && inode->block[12] != 0)
    {
        uint32_t start = block_idx - 12;
        uint32_t to_read = (count < ptrs_per_block - start) ? count : ptrs_per_block - start;

        uint32_t* indirect_block = read_block(fs, inode->block[12]);
        kmemcpy(blocks + blocks_read, indirect_block + start, to_read * sizeof(uint32_t));
        kfree(indirect_block);

        blocks_read += to_read;
        block_idx += to_read;
        count -= to_read;
    }

    // Double indirect block (simplified - doesn't handle full traversal)
    if (count > 0 && block_idx < 12 + ptrs_per_block + ptrs_per_block * ptrs_per_block &&
        inode->block[13] != 0)
    {
        uint32_t start = block_idx - 12 - ptrs_per_block;
        uint32_t first_level = start / ptrs_per_block;
        uint32_t second_level = start % ptrs_per_block;

        uint32_t* first_indirect = read_block(fs, inode->block[13]);
        if (first_level < ptrs_per_block && first_indirect[first_level] != 0)
        {
            uint32_t* second_indirect = read_block(fs, first_indirect[first_level]);

            uint32_t to_read =
                (count < ptrs_per_block - second_level) ? count : ptrs_per_block - second_level;
            kmemcpy(blocks + blocks_read, second_indirect + second_level,
                    to_read * sizeof(uint32_t));

            kfree(second_indirect);
            blocks_read += to_read;
        }

        kfree(first_indirect);
    }

    return blocks_read;
}

static int write_block_pointers(ext2_fs_t* fs,
                                struct ext2_inode* inode,
                                uint32_t block_idx,
                                uint32_t* blocks,
                                uint32_t count)
{
    uint32_t blocks_written = 0;
    uint32_t ptrs_per_block = fs->block_size / sizeof(uint32_t);

    // Direct blocks
    if (block_idx < 12)
    {
        uint32_t to_write = (count < 12 - block_idx) ? count : 12 - block_idx;
        kmemcpy(&inode->block[block_idx], blocks, to_write * sizeof(uint32_t));
        blocks_written += to_write;
        block_idx += to_write;
        count -= to_write;
    }

    // Single indirect block
    if (count > 0 && block_idx < 12 + ptrs_per_block)
    {
        if (inode->block[12] == 0)
        {
            inode->block[12] = allocate_block(fs);
            if (inode->block[12] == 0)
            {
                return blocks_written;
            }

            // Initialize indirect block with zeros
            uint32_t* indirect_block = kmalloc(fs->block_size);
            kmemset(indirect_block, 0, fs->block_size);
            write_block(fs, inode->block[12], indirect_block);
            kfree(indirect_block);
        }

        uint32_t start = block_idx - 12;
        uint32_t to_write = (count < ptrs_per_block - start) ? count : ptrs_per_block - start;

        uint32_t* indirect_block = read_block(fs, inode->block[12]);
        kmemcpy(indirect_block + start, blocks + blocks_written, to_write * sizeof(uint32_t));
        write_block(fs, inode->block[12], indirect_block);
        kfree(indirect_block);

        blocks_written += to_write;
        block_idx += to_write;
        count -= to_write;
    }

    // Double indirect block (simplified)
    if (count > 0 && block_idx < 12 + ptrs_per_block + ptrs_per_block * ptrs_per_block)
    {
        if (inode->block[13] == 0)
        {
            inode->block[13] = allocate_block(fs);
            if (inode->block[13] == 0)
            {
                return blocks_written;
            }

            // Initialize double indirect block with zeros
            uint32_t* first_indirect = kmalloc(fs->block_size);
            kmemset(first_indirect, 0, fs->block_size);
            write_block(fs, inode->block[13], first_indirect);
            kfree(first_indirect);
        }

        uint32_t start = block_idx - 12 - ptrs_per_block;
        uint32_t first_level = start / ptrs_per_block;
        uint32_t second_level = start % ptrs_per_block;

        uint32_t* first_indirect = read_block(fs, inode->block[13]);
        if (first_indirect[first_level] == 0)
        {
            first_indirect[first_level] = allocate_block(fs);
            if (first_indirect[first_level] == 0)
            {
                kfree(first_indirect);
                return blocks_written;
            }

            // Initialize second level indirect block with zeros
            uint32_t* second_indirect = kmalloc(fs->block_size);
            kmemset(second_indirect, 0, fs->block_size);
            write_block(fs, first_indirect[first_level], second_indirect);
            kfree(second_indirect);

            write_block(fs, inode->block[13], first_indirect);
        }

        uint32_t to_write =
            (count < ptrs_per_block - second_level) ? count : ptrs_per_block - second_level;

        uint32_t* second_indirect = read_block(fs, first_indirect[first_level]);
        kmemcpy(second_indirect + second_level, blocks + blocks_written,
                to_write * sizeof(uint32_t));
        write_block(fs, first_indirect[first_level], second_indirect);
        kfree(second_indirect);

        blocks_written += to_write;
        kfree(first_indirect);
    }

    return blocks_written;
}

static uint32_t count_blocks_needed(ext2_fs_t* fs, uint32_t size)
{
    return (size + fs->block_size - 1) / fs->block_size;
}

static int
ensure_blocks_allocated(ext2_fs_t* fs, struct ext2_inode* inode, uint32_t required_blocks)
{
    uint32_t current_blocks = count_blocks_needed(fs, inode->size);

    if (required_blocks <= current_blocks)
    {
        return 0;
    }

    uint32_t blocks_to_allocate = required_blocks - current_blocks;
    uint32_t* blocks = kmalloc(blocks_to_allocate * sizeof(uint32_t));

    // Allocate new blocks
    for (uint32_t i = 0; i < blocks_to_allocate; i++)
    {
        blocks[i] = allocate_block(fs);
        if (blocks[i] == 0)
        {
            // Free any blocks we already allocated
            for (uint32_t j = 0; j < i; j++)
            {
                free_block(fs, blocks[j]);
            }
            kfree(blocks);
            return -1;
        }
    }

    // Write the new block pointers
    if (write_block_pointers(fs, inode, current_blocks, blocks, blocks_to_allocate) !=
        blocks_to_allocate)
    {
        // Free all blocks we allocated
        for (uint32_t i = 0; i < blocks_to_allocate; i++)
        {
            free_block(fs, blocks[i]);
        }
        kfree(blocks);
        return -1;
    }

    kfree(blocks);
    return 0;
}