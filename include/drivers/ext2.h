/**
 * @file ext2.h
 * @brief EXT2 Filesystem Definitions and Structures
 *
 * Contains all structures, constants, and function prototypes
 * for working with the EXT2 filesystem.
 */

#ifndef EXT2_H
#define EXT2_H

#include <stddef.h>
#include <stdint.h>

#define SECTOR_SIZE 512       ///< Standard disk sector size
#define EXT2_SIGNATURE 0xEF53 ///< Magic number identifying EXT2 filesystem
#define EXT2_ROOT_INO 2       ///< Inode number of root directory

/**
 * @enum entry_type_t
 * @brief File type identifiers for directory entries
 */
typedef enum entry_type_t
{
    EXT2_FT_UNKNOWN,  ///< Unknown file type
    EXT2_FT_REG_FILE, ///< Regular file
    EXT2_FT_DIR,      ///< Directory
    EXT2_FT_CHRDEV,   ///< Character device
    EXT2_FT_BLKDEV,   ///< Block device
    EXT2_FT_FIFO,     ///< FIFO (named pipe)
    EXT2_FT_SOCK,     ///< Socket
    EXT2_FT_SYMLINK,  ///< Symbolic link
} entry_type_t;

/* File mode bitmasks */
#define EXT2_S_IFSOCK 0xC000 ///< Socket
#define EXT2_S_IFLNK 0xA000  ///< Symbolic link
#define EXT2_S_IFREG 0x8000  ///< Regular file
#define EXT2_S_IFBLK 0x6000  ///< Block device
#define EXT2_S_IFDIR 0x4000  ///< Directory
#define EXT2_S_IFCHR 0x2000  ///< Character device
#define EXT2_S_IFIFO 0x1000  ///< FIFO
#define EXT2_S_ISUID 0x0800  ///< Set UID bit
#define EXT2_S_ISGID 0x0400  ///< Set GID bit
#define EXT2_S_ISVTX 0x0200  ///< Sticky bit
#define EXT2_S_IRUSR 0x0100  ///< User read permission
#define EXT2_S_IWUSR 0x0080  ///< User write permission
#define EXT2_S_IXUSR 0x0040  ///< User execute permission
#define EXT2_S_IRGRP 0x0020  ///< Group read permission
#define EXT2_S_IWGRP 0x0010  ///< Group write permission
#define EXT2_S_IXGRP 0x0008  ///< Group execute permission
#define EXT2_S_IROTH 0x0004  ///< Others read permission
#define EXT2_S_IWOTH 0x0002  ///< Others write permission
#define EXT2_S_IXOTH 0x0001  ///< Others execute permission

/* Seek mode constants */
#define SEEK_SET 0 ///< Seek from beginning of file
#define SEEK_CUR 1 ///< Seek from current position
#define SEEK_END 2 ///< Seek from end of file

/**
 * @struct ext2_superblock
 * @brief EXT2 Superblock structure
 *
 * Contains filesystem metadata and configuration parameters
 */
typedef struct ext2_superblock
{
    uint32_t inodes_count;       ///< Total number of inodes
    uint32_t blocks_count;       ///< Total number of blocks
    uint32_t r_blocks_count;     ///< Number of reserved blocks
    uint32_t free_blocks_count;  ///< Number of free blocks
    uint32_t free_inodes_count;  ///< Number of free inodes
    uint32_t first_data_block;   ///< First data block number
    uint32_t log_block_size;     ///< Block size (log2(block_size) - 10)
    uint32_t log_frag_size;      ///< Fragment size (obsolete in EXT2)
    uint32_t blocks_per_group;   ///< Blocks per block group
    uint32_t frags_per_group;    ///< Fragments per block group
    uint32_t inodes_per_group;   ///< Inodes per block group
    uint32_t mtime;              ///< Last mount time
    uint32_t wtime;              ///< Last write time
    uint16_t mnt_count;          ///< Mount count
    uint16_t max_mnt_count;      ///< Maximum mount count
    uint16_t magic;              ///< Magic signature (0xEF53)
    uint16_t state;              ///< Filesystem state
    uint16_t errors;             ///< Behavior when detecting errors
    uint16_t minor_rev_level;    ///< Minor revision level
    uint32_t lastcheck;          ///< Time of last consistency check
    uint32_t checkinterval;      ///< Interval between forced checks
    uint32_t creator_os;         ///< OS that created filesystem
    uint32_t rev_level;          ///< Revision level
    uint16_t def_resuid;         ///< Default UID for reserved blocks
    uint16_t def_resgid;         ///< Default GID for reserved blocks
    uint32_t first_ino;          ///< First non-reserved inode
    uint16_t inode_size;         ///< Size of inode structure
    uint16_t block_group_nr;     ///< Block group number (for backups)
    uint32_t feature_compat;     ///< Compatible feature set
    uint32_t feature_incompat;   ///< Incompatible feature set
    uint32_t feature_ro_compat;  ///< Read-only compatible feature set
    uint8_t uuid[16];            ///< 128-bit filesystem UUID
    char volume_name[16];        ///< Volume name
    char last_mounted[64];       ///< Path where last mounted
    uint32_t algo_bitmap;        ///< Algorithm usage bitmap
    uint8_t prealloc_blocks;     ///< Number of blocks to preallocate
    uint8_t prealloc_dir_blocks; ///< Number of blocks to preallocate for directories
    uint16_t padding;            ///< Padding to make structure size 1024 bytes
    uint8_t journal_uuid[16];    ///< UUID of journal
    uint32_t journal_inum;       ///< Inode number of journal file
    uint32_t journal_dev;        ///< Device number of journal
    uint32_t last_orphan;        ///< Start of orphan inode list
    uint32_t hash_seed[4];       ///< HTREE hash seed
    uint8_t def_hash_version;    ///< Default hash algorithm
    uint8_t jnl_backup_type;     ///< Type of journal backup
    uint16_t desc_size;          ///< Group descriptor size
    uint32_t default_mount_opts; ///< Default mount options
    uint32_t first_meta_bg;      ///< First meta block group
    uint32_t mkfs_time;          ///< When filesystem was created
    uint32_t jnl_blocks[17];     ///< Backup journal inodes and blocks
} ext2_superblock;

/**
 * @struct ext2_bg_desc
 * @brief Block Group Descriptor structure
 *
 * Contains information about a block group's layout
 */
typedef struct ext2_bg_desc
{
    uint32_t block_bitmap;      ///< Block number of block bitmap
    uint32_t inode_bitmap;      ///< Block number of inode bitmap
    uint32_t inode_table;       ///< First block of inode table
    uint16_t free_blocks_count; ///< Number of free blocks in group
    uint16_t free_inodes_count; ///< Number of free inodes in group
    uint16_t used_dirs_count;   ///< Number of directories in group
    uint16_t pad;               ///< Padding
    uint32_t reserved[3];       ///< Reserved for future use
} ext2_bg_desc;

/**
 * @struct ext2_inode
 * @brief EXT2 Inode structure
 *
 * Contains metadata about a file and pointers to its data blocks
 */
typedef struct ext2_inode
{
    uint16_t mode;        ///< File type and permissions
    uint16_t uid;         ///< Owner UID (lower 16 bits)
    uint32_t size;        ///< File size in bytes
    uint32_t atime;       ///< Last access time
    uint32_t ctime;       ///< Creation time
    uint32_t mtime;       ///< Last modification time
    uint32_t dtime;       ///< Deletion time
    uint16_t gid;         ///< Group ID (lower 16 bits)
    uint16_t links_count; ///< Hard link count
    uint32_t blocks;      ///< Number of 512-byte blocks allocated
    uint32_t flags;       ///< File flags
    uint32_t osd1;        ///< OS dependent value
    uint32_t block[15];   ///< Pointers to data blocks
    uint32_t generation;  ///< File version (for NFS)
    uint32_t file_acl;    ///< Extended attributes block
    uint32_t dir_acl;     ///< Access control list
    uint32_t faddr;       ///< Fragment address
    uint8_t osd2[12];     ///< Additional OS dependent data
} ext2_inode;

/**
 * @struct ext2_dirent_t
 * @brief Directory Entry structure
 *
 * Represents a single entry in a directory
 */
typedef struct
{
    uint32_t inode;    ///< Inode number
    uint16_t rec_len;  ///< Length of this record
    uint8_t name_len;  ///< Length of name field
    uint8_t file_type; ///< File type (from entry_type_t)
    char name[];       ///< File name (variable length)
} ext2_dirent_t;

/**
 * @struct ext2_dirent_iter_t
 * @brief Directory Entry Iterator
 *
 * State for iterating through directory entries
 */
typedef struct
{
    void* buffer;              ///< Current block buffer
    size_t pos;                ///< Current position in buffer
    size_t block_remaining;    ///< Bytes remaining in current block
    uint32_t current_block;    ///< Current block number
    uint32_t blocks_remaining; ///< Blocks remaining to read
    uint32_t inode;            ///< Inode of directory being read
} ext2_dirent_iter_t;

/**
 * @struct ext2_fs_t
 * @brief EXT2 Filesystem Context
 *
 * Contains state and function pointers for working with an EXT2 filesystem
 */
typedef struct
{
    void* (*read_sectors)(uint32_t lba, uint32_t count);             ///< Sector read function
    void (*write_sectors)(uint32_t lba, uint32_t count, void* data); ///< Sector write function
    uint32_t start_sector;                                           ///< Starting sector of filesystem
    uint32_t end_sector;                                             ///< Ending sector of filesystem
    uint32_t block_size;                                             ///< Filesystem block size
    uint32_t blocks_per_group;                                       ///< Blocks per block group
    uint32_t inodes_per_group;                                       ///< Inodes per block group
    uint32_t inode_size;                                             ///< Size of each inode
    uint32_t bgdt_block;                                             ///< Block containing block group descriptor table
    uint32_t first_data_block;                                       ///< First data block number
    uint32_t total_blocks;                                           ///< Total blocks in filesystem
    uint32_t total_inodes;                                           ///< Total inodes in filesystem
    uint32_t groups_count;                                           ///< Number of block groups
    void* block_buffer;                                              ///< Temporary block buffer
} ext2_fs_t;

/* Forward declaration */
typedef struct file_descriptor_t file_descriptor_t;

/* ==================== Filesystem Operations ==================== */

/**
 * @brief Initialize EXT2 filesystem
 * @param fs Filesystem context to initialize
 * @param read_fn Function to read disk sectors
 * @param write_fn Function to write disk sectors
 * @param start Starting sector of filesystem
 * @param end Ending sector of filesystem
 * @return 0 on success, -1 on failure
 */
int ext2_init(ext2_fs_t* fs, void* (*read_fn)(uint32_t, uint32_t), void (*write_fn)(uint32_t, uint32_t, void*), uint32_t start, uint32_t end);

/**
 * @brief Clean up filesystem resources
 * @param fs Filesystem context
 */
void ext2_cleanup(ext2_fs_t* fs);

/* ==================== File Operations ==================== */

/**
 * @brief Open a file
 * @param fs Filesystem context
 * @param entry File descriptor to populate
 * @return 0 on success, -1 on failure
 */
int ext2_file_open(ext2_fs_t* fs, file_descriptor_t* entry);

/**
 * @brief Create a new file
 * @param fs Filesystem context
 * @param dir_inode Inode of parent directory
 * @param filename Name of new file
 * @param mode File permissions
 * @return 0 on success, -1 on failure
 */
int ext2_file_create(ext2_fs_t* fs, uint32_t dir_inode, const char* filename, uint16_t mode);

/**
 * @brief Close a file
 * @param fs Filesystem context
 * @param file File descriptor
 * @return 0 on success, -1 on failure
 */
int ext2_file_close(ext2_fs_t* fs, file_descriptor_t* file);

/**
 * @brief Read from a file
 * @param fs Filesystem context
 * @param file File descriptor
 * @param buf Buffer to store read data
 * @param count Number of bytes to read
 * @return Number of bytes read, or -1 on error
 */
long ext2_file_read(ext2_fs_t* fs, file_descriptor_t* file, void* buf, size_t count);

/**
 * @brief Write to a file
 * @param fs Filesystem context
 * @param file File descriptor
 * @param buf Data to write
 * @param count Number of bytes to write
 * @return Number of bytes written, or -1 on error
 */
long ext2_file_write(ext2_fs_t* fs, file_descriptor_t* file, const void* buf, size_t count);

/**
 * @brief Seek within a file
 * @param file File descriptor
 * @param offset Offset to seek to
 * @param whence SEEK_SET, SEEK_CUR, or SEEK_END
 * @return 0 on success, -1 on failure
 */
int ext2_file_seek(file_descriptor_t* file, long offset, int whence);

/**
 * @brief Truncate or extend a file
 * @param fs Filesystem context
 * @param file File descriptor
 * @param length New file length
 * @return 0 on success, -1 on failure
 */
int ext2_file_truncate(ext2_fs_t* fs, file_descriptor_t* file, size_t length);

/**
 * @brief Delete a file
 * @param fs Filesystem context
 * @param dir_inode Inode of parent directory
 * @param filename Name of file to delete
 * @return 0 on success, -1 on failure
 */
int ext2_file_delete(ext2_fs_t* fs, uint32_t dir_inode, const char* filename);

/* ==================== Directory Operations ==================== */

/**
 * @brief Create a new directory
 * @param fs Filesystem context
 * @param parent_inode Inode of parent directory
 * @param dirname Name of new directory
 * @param mode Directory permissions
 * @return 0 on success, -1 on failure
 */
int ext2_dir_create(ext2_fs_t* fs, uint32_t parent_inode, const char* dirname, uint16_t mode);

/**
 * @brief Delete a directory
 * @param fs Filesystem context
 * @param parent_inode Inode of parent directory
 * @param dirname Name of directory to delete
 * @return 0 on success, -1 on failure
 */
int ext2_dir_delete(ext2_fs_t* fs, uint32_t parent_inode, const char* dirname);

/**
 * @brief Start directory iteration
 * @param fs Filesystem context
 * @param iter Iterator to initialize
 * @param inode_num Inode of directory to iterate
 * @return 0 on success, -1 on failure
 */
int ext2_dir_iter_start(ext2_fs_t* fs, ext2_dirent_iter_t* iter, uint32_t inode_num);

/**
 * @brief Get next directory entry
 * @param fs Filesystem context
 * @param iter Directory iterator
 * @param dirent Pointer to store next directory entry
 * @return 0 on success, -1 on failure or end of directory
 */
int ext2_dir_iter_next(ext2_fs_t* fs, ext2_dirent_iter_t* iter, ext2_dirent_t** dirent);

/**
 * @brief Clean up directory iterator
 * @param iter Directory iterator to clean up
 */
void ext2_dir_iter_end(ext2_dirent_iter_t* iter);

/**
 * @brief Count entries in a directory
 * @param fs Filesystem context
 * @param dir_inode Inode of directory
 * @return Number of entries, or -1 on error
 */
int ext2_dir_count_entries(ext2_fs_t* fs, uint32_t dir_inode);

/* ==================== Utility Functions ==================== */

/**
 * @brief Get file/directory status
 * @param fs Filesystem context
 * @param inode_num Inode to examine
 * @param inode_out Buffer to store inode data
 * @return 0 on success, -1 on failure
 */
int ext2_stat(ext2_fs_t* fs, uint32_t inode_num, struct ext2_inode* inode_out);

/**
 * @brief Rename a file/directory
 * @param fs Filesystem context
 * @param old_dir_inode Source directory inode
 * @param new_dir_inode Destination directory inode
 * @param old_filename Original name
 * @param new_filename New name
 * @return 0 on success, -1 on failure
 */
int ext2_rename(ext2_fs_t* fs, uint32_t old_dir_inode, uint32_t new_dir_inode, const char* old_filename, const char* new_filename);

/**
 * @brief Get file size
 * @param fs Filesystem context
 * @param path Path to file
 * @param size_out Pointer to store size
 * @return 0 on success, -1 on failure
 */
int ext2_get_size(ext2_fs_t* fs, const char* path, uint32_t* size_out);

/**
 * @brief Check if path is a directory
 * @param fs Filesystem context
 * @param path Path to check
 * @return 1 if directory, 0 if not, -1 on error
 */
int ext2_is_dir(ext2_fs_t* fs, const char* path);

/**
 * @brief Check if path is a regular file
 * @param fs Filesystem context
 * @param path Path to check
 * @return 1 if file, 0 if not, -1 on error
 */
int ext2_is_file(ext2_fs_t* fs, const char* path);

/**
 * @brief Read inode data
 * @param fs Filesystem context
 * @param inode_num Inode number to read
 * @param inode Buffer to store inode data
 * @return 0 on success, -1 on failure
 */
int ext2_read_inode(ext2_fs_t* fs, uint32_t inode_num, struct ext2_inode* inode);

#endif // EXT2_H