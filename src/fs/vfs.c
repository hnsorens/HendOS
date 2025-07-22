/**
 * @file vfs.c
 * @brief Virtual Filesystem Switch Implementation
 *
 * Implements the core functionality of the virtual filesystem layer,
 * including path resolution, directory traversal, and filesystem operations.
 */

#include <fs/vfs.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <misc/debug.h>

/* IDE Controller Registers */
#define IDE_CMD_REG 0x1F7
#define IDE_STATUS_REG 0x1F7
#define IDE_DATA_REG 0x1F0

/**
 * @struct GPTHeader
 * @brief GUID Partition Table header structure
 */
typedef struct
{
    char signature[8];                ///< "EFI PART" signature
    uint32_t revision;                ///< Header revision
    uint32_t header_size;             ///< Header size in bytes
    uint32_t header_crc32;            ///< Header CRC32 checksum
    uint32_t reserved;                ///< Reserved field
    uint64_t current_lba;             ///< LBA of this header
    uint64_t backup_lba;              ///< LBA of backup header
    uint64_t first_usable_lba;        ///< First usable LBA for partitions
    uint64_t last_usable_lba;         ///< Last usable LBA for partitions
    uint8_t disk_guid[16];            ///< Disk GUID
    uint64_t partition_entries_lba;   ///< LBA of partition entries
    uint32_t num_partition_entries;   ///< Number of partition entries
    uint32_t size_of_partition_entry; ///< Size of each partition entry
    uint32_t partition_entries_crc32; ///< CRC32 of partition entries
    uint8_t reserved2[420];           ///< Padding to 512 bytes
} __attribute__((packed)) GPTHeader;

/**
 * @struct GPTPartitionEntry
 * @brief GPT partition entry structure
 */
typedef struct
{
    uint8_t type_guid[16];   ///< Partition type GUID
    uint8_t unique_guid[16]; ///< Unique partition GUID
    uint64_t first_lba;      ///< First LBA of partition
    uint64_t last_lba;       ///< Last LBA of partition
    uint64_t attributes;     ///< Partition attributes
    uint16_t name[36];       ///< Partition name in UTF-16LE
} __attribute__((packed)) GPTPartitionEntry;

/**
 * @brief Wait for IDE disk to be ready
 */
void wait_for_disk_ready()
{
    unsigned char status;
    do
    {
        status = inb(IDE_CMD_REG); /* Read status register */
    } while (status & 0x80);       /* Wait until busy bit is clear */
}

/**
 * @brief Read sectors from disk
 * @param lba Starting Logical Block Address
 * @param sector_count Number of sectors to read
 * @return Buffer containing sector data
 */
uint8_t* KERNEL_ReadSectors(uint32_t lba, uint32_t sector_count)
{
    uint8_t* buffer = kmalloc(sizeof(uint8_t) * 512 * sector_count);

    /* Wait for disk to be ready */
    wait_for_disk_ready();

    /* Set up LBA addressing */
    outb(0x1F2, sector_count);                /* Sector count */
    outb(0x1F3, lba & 0xFF);                  /* LBA bits 0-7 */
    outb(0x1F4, (lba >> 8) & 0xFF);           /* LBA bits 8-15 */
    outb(0x1F5, (lba >> 16) & 0xFF);          /* LBA bits 16-23 */
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); /* LBA bits 24-27 + drive select */
    outb(0x1F7, 0x20);                        /* READ SECTORS command */

    /* Read data for each sector */
    for (int s = 0; s < sector_count; ++s)
    {
        wait_for_disk_ready();

        /* Read 256 words (512 bytes) per sector */
        for (int i = 0; i < 256; ++i)
        {
            uint16_t data = inw(0x1F0);
            buffer[s * 512 + i * 2] = data & 0xFF;
            buffer[s * 512 + i * 2 + 1] = data >> 8;
        }
    }

    return buffer;
}

/**
 * @brief Write sectors to disk
 * @param lba Starting Logical Block Address
 * @param sector_count Number of sectors to write
 * @param data Buffer containing data to write
 */
void KERNEL_WriteSectors(uint32_t lba, uint32_t sector_count, const void* data)
{
    const uint8_t* buffer = (const uint8_t*)data;

    wait_for_disk_ready();

    /* Set up LBA addressing */
    outb(0x1F2, sector_count);                /* Sector count */
    outb(0x1F3, lba & 0xFF);                  /* LBA bits 0-7 */
    outb(0x1F4, (lba >> 8) & 0xFF);           /* LBA bits 8-15 */
    outb(0x1F5, (lba >> 16) & 0xFF);          /* LBA bits 16-23 */
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); /* LBA bits 24-27 + drive select */
    outb(0x1F7, 0x30);                        /* WRITE SECTORS command */

    /* Write data for each sector */
    for (int s = 0; s < sector_count; ++s)
    {
        wait_for_disk_ready();

        /* Write 256 words (512 bytes) per sector */
        for (int i = 0; i < 256; ++i)
        {
            uint16_t word = buffer[s * 512 + i * 2] | (buffer[s * 512 + i * 2 + 1] << 8);
            outw(0x1F0, word);
        }
    }

    /* Flush write cache */
    outb(0x1F7, 0xE7); /* FLUSH CACHE command */
    wait_for_disk_ready();
}

/**
 * @struct GPTPartition
 * @brief Simplified GPT partition structure
 */
typedef struct
{
    uint8_t type_guid[16];   ///< Partition type GUID
    uint8_t unique_guid[16]; ///< Unique partition GUID
    uint64_t first_lba;      ///< First LBA of partition
    uint64_t last_lba;       ///< Last LBA of partition
    uint64_t attributes;     ///< Partition attributes
    uint16_t name[36];       ///< Partition name in UTF-16
} __attribute__((packed)) GPTPartition;

/**
 * @brief Parse GPT partition table
 * @param header Valid GPT header structure
 * @param partitions Output pointer for partition array
 * @return Number of valid partitions found
 */
int parse_gpt_partitions(GPTHeader* header, GPTPartition** partitions)
{
    /* Calculate partition table location and size */
    uint64_t entries_lba = header->partition_entries_lba;
    uint32_t num_entries = header->num_partition_entries;
    uint32_t entry_size = header->size_of_partition_entry;

    /* Read partition entries from disk */
    uint8_t* entries = KERNEL_ReadSectors(entries_lba, (num_entries * entry_size + 511) / 512); // Round up to next sector
    /* Allocate output partition array */
    *partitions = (GPTPartition*)kmalloc(num_entries * sizeof(GPTPartition));
    int valid_partitions = 0;

    /* Process each partition entry */
    for (int i = 0; i < num_entries; ++i)
    {
        GPTPartitionEntry* entry = (GPTPartitionEntry*)(entries + i * entry_size);

        /* Skip empty entries */
        if (entry->type_guid[0] == 0)
        {
            continue;
        }

        /* Populate partition structure */
        GPTPartition* partition = &(*partitions)[valid_partitions];
        kmemcpy(partition->type_guid, entry->type_guid, 16);
        kmemcpy(partition->unique_guid, entry->unique_guid, 16);
        partition->first_lba = entry->first_lba;
        partition->last_lba = entry->last_lba;
        partition->attributes = entry->attributes;

        /* Convert UTF-16 name */
        for (int j = 0; j < 36 && entry->name[j] != 0; ++j)
        {
            partition->name[j] = (char)entry->name[j];
        }

        valid_partitions++;
    }

    return valid_partitions;
}

/**
 * @brief Parse GPT header and partition table
 * @param lba LBA of GPT header (usually 1)
 * @return Array of valid partitions
 */
GPTPartition* KERNEL_ParseGPTHeader(uint32_t lba)
{
    /* Read GPT header from disk */
    GPTHeader* header = (GPTHeader*)KERNEL_ReadSectors(lba, 1);
    if (header != NULL)
    {
        GPTPartition* partitions = NULL;
        int num_partitions = parse_gpt_partitions(header, &partitions);

        if (num_partitions > 0)
        {
            return partitions;
        }
    }
    return NULL;
}

/**
 * @brief Compute FNV-1a hash of string
 * @param str Input string
 * @return 32-bit hash value
 */
static uint32_t fnv1a_hash(const char* str)
{
    uint32_t hash = 2166136261u;
    while (*str)
    {
        hash ^= (uint8_t)(*str++);
        hash *= 16777619u;
    }
    return hash;
}

/**
 * @brief Initialize list head
 * @param list List head to initialize
 */
static void init_list_head(list_head_t* list)
{
    list->next = list;
    list->prev = list;
}

/**
 * @brief Find child entry by name
 * @param parent Parent directory entry
 * @param name Child name to find
 * @return Found child entry or NULL
 */
static vfs_entry_t* vfs_find_child(vfs_entry_t* parent, const char* name)
{
    list_head_t* pos;
    for (pos = parent->children.next; pos != &parent->children; pos = pos->next)
    {
        vfs_entry_t* child = container_of(pos, vfs_entry_t, sibling);
        if (strcmp(child->name, name) == 0)
        {
            return child;
        }
    }
    return NULL;
}

/**
 * @brief Add node to end of list
 * @param new_node Node to add
 * @param head List head
 */
static void list_add_tail(list_head_t* new_node, list_head_t* head)
{
    new_node->prev = head->prev;
    new_node->next = head;
    head->prev->next = new_node;
    head->prev = new_node;
}

/**
 * @brief Check if entry has children
 * @param entry VFS entry to check
 * @return 1 if has children, 0 otherwise
 */
static int vfs_entry_has_children(vfs_entry_t* entry)
{
    return entry->children.next != &entry->children;
}

/**
 * @brief Initialize VFS entry
 * @param entry Entry to initialize
 * @param name Name for entry
 */
void vfs_entry_init(vfs_entry_t* entry, const char* name)
{
    entry->name = kmalloc(kernel_strlen(name) + 1);
    kmemcpy(entry->name, name, kernel_strlen(name) + 1);
    entry->inode_num = 0;
    entry->parent = 0;
    entry->children_loaded = 0;
    entry->type = EXT2_FT_DIR;
    entry->name_hash = fnv1a_hash(name);
    init_list_head(&entry->children);
    init_list_head(&entry->sibling);
}

/**
 * @brief Add child to parent directory
 * @param parent Parent directory
 * @param child Child to add
 */
void vfs_add_child(vfs_entry_t* parent, vfs_entry_t* child)
{
    child->parent = parent;
    list_add_tail(&child->sibling, &parent->children);
}

/**
 * @brief Initialize VFS subsystem
 */
void vfs_init()
{
    /* Parse GPT partitions */
    GPTPartition* partitions = KERNEL_ParseGPTHeader(1);
    GPTPartition* filesystemPartition = &partitions[1];

    /* Initialize EXT2 filesystem */
    if (ext2_init(FILESYSTEM, KERNEL_ReadSectors, KERNEL_WriteSectors, filesystemPartition->first_lba, filesystemPartition->last_lba) != 0)
    {
        /* Handle initialization failure */
    }

    /* Initialize root directory */
    vfs_entry_init(ROOT, "");
    ROOT->inode_num = 2;

    /* Create /dev directory */
    *DEV = vfs_create_entry(ROOT, "dev", EXT2_FT_DIR);
    (*DEV)->children_loaded = 1;
}

size_t vfs_write_reg_file(file_descriptor_t* open_file, uint8_t* buf, size_t size)
{
    return ext2_file_write(FILESYSTEM, open_file, buf, size);
}

size_t vfs_read_reg_file(file_descriptor_t* open_file, uint8_t* buf, size_t size)
{
    return ext2_file_read(FILESYSTEM, open_file, buf, size);
}

/**
 * @brief Populate directory with entries
 * @param dir Directory to populate
 */
void vfs_populate_directory(vfs_entry_t* dir)
{
    if (dir->type != EXT2_FT_DIR || dir->children_loaded)
        return;
    /* Iterate through directory entries */
    ext2_dirent_t* dirent;
    ext2_dirent_iter_t iter;
    ext2_dir_iter_start(FILESYSTEM, &iter, dir->inode_num);

    while (ext2_dir_iter_next(FILESYSTEM, &iter, &dirent) == 0)
    {
        /* Create new VFS entry */
        vfs_entry_t* entry = pool_allocate(*VFS_ENTRY_POOL);
        vfs_entry_init(entry, dirent->name);
        entry->type = dirent->file_type;
        entry->inode_num = dirent->inode;
        entry->parent = dir;
        entry->inode = pool_allocate(*INODE_POOL);
        // TODO: Make a better way for dynamicall setting callbacks, fornow there are 8
        entry->ops = kmalloc(sizeof(void*) * 8);
        vfs_add_child(dir, entry);

        if (entry->type == EXT2_FT_REG_FILE)
        {
            entry->ops[DEV_WRITE] = vfs_write_reg_file;
            entry->ops[DEV_READ] = vfs_read_reg_file;
        }
    }

    ext2_dir_iter_end(&iter);
}

/**
 * @brief Find VFS entry by path
 * @param current Starting directory
 * @param out Output pointer for found entry
 * @param path Path to resolve
 * @return 0 on success, 1 on failure
 */
int vfs_find_entry(vfs_entry_t* current, vfs_entry_t** out, const char* path)
{
    if (!current || !path || !*path)
        return 1;

    /* Copy path to modifiable buffer */
    kernel_strcpy(PATH, path);
    char* path_ptr = PATH;

    /* Handle absolute paths */
    if (path_ptr[0] == '/')
    {
        current = ROOT;
        path_ptr++;
    }

    /* Process each path component */
    while (*path_ptr)
    {
        /* Extract next component */
        char* next_slash = path_ptr;
        while (*next_slash && *next_slash != '/')
            next_slash++;

        char component[256];
        size_t len = next_slash - path_ptr;
        if (len >= sizeof(component))
            return 1;

        kmemcpy(component, path_ptr, len);
        component[len] = '\0';

        /* Handle . and .. special directories */
        if (kernel_strcmp(component, ".") == 0)
        {
            /* Current directory - no action needed */
        }
        else if (kernel_strcmp(component, "..") == 0)
        {
            /* Parent directory */
            if (current->parent)
                current = current->parent;
        }
        else
        {
            /* Lazy-load children if needed */
            if (!current->children_loaded)
            {
                vfs_populate_directory(current);
                current->children_loaded = 1;
            }

            /* Find child entry */
            vfs_entry_t* next = vfs_find_child(current, component);
            if (!next)
            {
                return 1;
            }

            current = next;
        }

        path_ptr = *next_slash ? next_slash + 1 : next_slash;
    }

    *out = current;
    return 0;
}

/**
 * @brief Open file through VFS
 * @param entry VFS entry to open
 * @return Open file handle
 */
file_descriptor_t* vfs_open_file(vfs_entry_t* entry)
{
    return fdm_open_file(entry);
}

/**
 * @brief Create new VFS entry
 * @param dir Parent directory
 * @param name Entry name
 * @param type Entry type
 * @return Created entry
 */
vfs_entry_t* vfs_create_entry(vfs_entry_t* dir, const char* name, entry_type_t type)
{

    vfs_entry_t* entry = pool_allocate(*VFS_ENTRY_POOL);
    vfs_entry_init(entry, name);
    entry->type = type;
    entry->inode_num = -1;
    entry->parent = dir;
    entry->inode = pool_allocate(*INODE_POOL);
    // TODO: Make a better way for dynamicall setting callbacks, fornow there are 8
    entry->ops = kmalloc(sizeof(void*) * 8);
    vfs_add_child(dir, entry);
    return entry;
}

/**
 * @brief Build full path for entry
 * @param dir Entry to get path for
 * @param buffer Output buffer
 * @param offset Current buffer offset
 */
void vfs_path(vfs_entry_t* dir, char* buffer, uint64_t* offset)
{
    if (dir->parent)
    {
        vfs_path(dir->parent, buffer, offset);
    }

    kernel_strcat(&buffer[*offset], dir->name);
    *offset += kernel_strlen(dir->name);
    buffer[*offset] = '\0';
    kernel_strcat(&buffer[*offset], "/");
    *offset += 1;
    buffer[*offset] = '\0';
}