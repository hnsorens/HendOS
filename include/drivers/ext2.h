/**
 * @file ext2.h
 * @brief EXT2 Filesystem Driver Interface
 *
 * Defines structures, constants, and function prototypes for EXT2 filesystem operations.
 */
#ifndef EXT2_H
#define EXT2_H

#include <stddef.h>
#include <stdint.h>
// #include <sys/types.h>
// #include <time.h>

#define SECTOR_SIZE 512
#define EXT2_SIGNATURE 0xEF53
#define EXT2_ROOT_INO 2

// File types
typedef enum entry_type_t
{
    EXT2_FT_UNKNOWN,
    EXT2_FT_REG_FILE,
    EXT2_FT_DIR,
    EXT2_FT_CHRDEV,
    EXT2_FT_BLKDEV,
    EXT2_FT_FIFO,
    EXT2_FT_SOCK,
    EXT2_FT_SYMLINK,
} entry_type_t;

// File modes
#define EXT2_S_IFSOCK 0xC000
#define EXT2_S_IFLNK 0xA000
#define EXT2_S_IFREG 0x8000
#define EXT2_S_IFBLK 0x6000
#define EXT2_S_IFDIR 0x4000
#define EXT2_S_IFCHR 0x2000
#define EXT2_S_IFIFO 0x1000
#define EXT2_S_ISUID 0x0800
#define EXT2_S_ISGID 0x0400
#define EXT2_S_ISVTX 0x0200
#define EXT2_S_IRUSR 0x0100
#define EXT2_S_IWUSR 0x0080
#define EXT2_S_IXUSR 0x0040
#define EXT2_S_IRGRP 0x0020
#define EXT2_S_IWGRP 0x0010
#define EXT2_S_IXGRP 0x0008
#define EXT2_S_IROTH 0x0004
#define EXT2_S_IWOTH 0x0002
#define EXT2_S_IXOTH 0x0001

#define SEEK_SET 0 /* Seek from beginning of file.  */
#define SEEK_CUR 1 /* Seek from current position.  */
#define SEEK_END 2 /* Seek from end of file.  */

// Superblock structure
typedef struct ext2_superblock
{
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t r_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t log_frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;
    uint32_t mtime;
    uint32_t wtime;
    uint16_t mnt_count;
    uint16_t max_mnt_count;
    uint16_t magic;
    uint16_t state;
    uint16_t errors;
    uint16_t minor_rev_level;
    uint32_t lastcheck;
    uint32_t checkinterval;
    uint32_t creator_os;
    uint32_t rev_level;
    uint16_t def_resuid;
    uint16_t def_resgid;
    uint32_t first_ino;
    uint16_t inode_size;
    uint16_t block_group_nr;
    uint32_t feature_compat;
    uint32_t feature_incompat;
    uint32_t feature_ro_compat;
    uint8_t uuid[16];
    char volume_name[16];
    char last_mounted[64];
    uint32_t algo_bitmap;
    uint8_t prealloc_blocks;
    uint8_t prealloc_dir_blocks;
    uint16_t padding;
    uint8_t journal_uuid[16];
    uint32_t journal_inum;
    uint32_t journal_dev;
    uint32_t last_orphan;
    uint32_t hash_seed[4];
    uint8_t def_hash_version;
    uint8_t jnl_backup_type;
    uint16_t desc_size;
    uint32_t default_mount_opts;
    uint32_t first_meta_bg;
    uint32_t mkfs_time;
    uint32_t jnl_blocks[17];
} ext2_superblock;

// Block group descriptor
typedef struct ext2_bg_desc
{
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t used_dirs_count;
    uint16_t pad;
    uint32_t reserved[3];
} ext2_bg_desc;

// Inode structure
typedef struct ext2_inode
{
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t links_count;
    uint32_t blocks;
    uint32_t flags;
    uint32_t osd1;
    uint32_t block[15];
    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t faddr;
    uint8_t osd2[12];
} ext2_inode;

// Directory entry
typedef struct
{
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[];
} ext2_dirent_t;

// Directory entry iterator
typedef struct
{
    void* buffer;
    size_t pos;
    size_t block_remaining;
    uint32_t current_block;
    uint32_t blocks_remaining;
    uint32_t inode;
} ext2_dirent_iter_t;

// Filesystem context
typedef struct
{
    void* (*read_sectors)(uint32_t lba, uint32_t count);
    void (*write_sectors)(uint32_t lba, uint32_t count, void* data);
    uint32_t start_sector;
    uint32_t end_sector;
    uint32_t block_size;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint32_t inode_size;
    uint32_t bgdt_block;
    uint32_t first_data_block;
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t groups_count;
    void* block_buffer;
} ext2_fs_t;

typedef struct file_descriptor_t file_descriptor_t;

// Initialize filesystem
int ext2_init(ext2_fs_t* fs, void* (*read_fn)(uint32_t, uint32_t), void (*write_fn)(uint32_t, uint32_t, void*), uint32_t start, uint32_t end);

// Clean up filesystem
void ext2_cleanup(ext2_fs_t* fs);

// File operations
int ext2_file_open(ext2_fs_t* fs, file_descriptor_t* entry);
int ext2_file_create(ext2_fs_t* fs, uint32_t dir_inode, const char* filename, uint16_t mode);
int ext2_file_close(ext2_fs_t* fs, file_descriptor_t* file);
long ext2_file_read(ext2_fs_t* fs, file_descriptor_t* file, void* buf, size_t count);
long ext2_file_write(ext2_fs_t* fs, file_descriptor_t* file, const void* buf, size_t count);
int ext2_file_seek(file_descriptor_t* file, long offset, int whence);
int ext2_file_truncate(ext2_fs_t* fs, file_descriptor_t* file, size_t length);
int ext2_file_delete(ext2_fs_t* fs, uint32_t dir_inode, const char* filename);

// Directory operations
int ext2_dir_create(ext2_fs_t* fs, uint32_t parent_inode, const char* dirname, uint16_t mode);
int ext2_dir_delete(ext2_fs_t* fs, uint32_t parent_inode, const char* dirname);
int ext2_dir_iter_start(ext2_fs_t* fs, ext2_dirent_iter_t* iter, uint32_t inode_num);
int ext2_dir_iter_next(ext2_fs_t* fs, ext2_dirent_iter_t* iter, ext2_dirent_t** dirent);
void ext2_dir_iter_end(ext2_dirent_iter_t* iter);
int ext2_dir_count_entries(ext2_fs_t* fs, uint32_t dir_inode);

// Utility functions
int ext2_stat(ext2_fs_t* fs, uint32_t inode_num, struct ext2_inode* inode_out);
int ext2_rename(ext2_fs_t* fs, uint32_t old_dir_inode, uint32_t new_dir_inode, const char* old_filename, const char* new_filename);
int ext2_get_size(ext2_fs_t* fs, const char* path, uint32_t* size_out);
int ext2_is_dir(ext2_fs_t* fs, const char* path);
int ext2_is_file(ext2_fs_t* fs, const char* path);
int ext2_read_inode(ext2_fs_t* fs, uint32_t inode_num, struct ext2_inode* inode);

#endif // EXT2_H