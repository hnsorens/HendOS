#include <fs/vfs.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <misc/debug.h>

#define IDE_CMD_REG 0x1F7
#define IDE_STATUS_REG 0x1F7
#define IDE_DATA_REG 0x1F0

typedef struct
{
    char signature[8]; // "EFI PART"
    uint32_t revision; // Usually 0x00010000
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;      // LBA of this header (usually 1)
    uint64_t backup_lba;       // LBA of backup header
    uint64_t first_usable_lba; // Start of usable space
    uint64_t last_usable_lba;
    uint8_t disk_guid[16];            // Disk GUID (unique to the disk)
    uint64_t partition_entries_lba;   // LBA of partition entries (usually 2)
    uint32_t num_partition_entries;   // Number of partition entries (usually 128)
    uint32_t size_of_partition_entry; // Size of each partition entry (usually 128)
    uint32_t partition_entries_crc32; // CRC of the partition entries
    uint8_t reserved2[420];           // Fills up the rest of the 512 bytes
} __attribute__((packed)) GPTHeader;

typedef struct
{
    uint8_t type_guid[16];   // Partition type GUID (e.g., EFI System, Linux, etc.)
    uint8_t unique_guid[16]; // Unique GUID for this partition
    uint64_t first_lba;      // Starting LBA of the partition
    uint64_t last_lba;       // Ending LBA of the partition
    uint64_t attributes;     // Partition attributes (usually 0)
    uint16_t name[36];       // Partition name in UTF-16LE (can be empty)
} __attribute__((packed)) GPTPartitionEntry;

void wait_for_disk_ready()
{
    unsigned char status;
    do
    {
        status = inb(IDE_CMD_REG); // Read status register
    } while (status & 0x80);       // Wait until the busy bit is clear
}

uint8_t* KERNEL_ReadSectors(uint32_t lba, uint32_t sector_count)
{
    uint8_t* buffer = kmalloc(sizeof(uint8_t) * 512 * sector_count);
    // Step 1: Select the drive (Primary master)
    wait_for_disk_ready();

    // Set up the parameters for reading a sector (assumes we are reading sector 0)
    outb(0x1F2, sector_count);                // Number of sectors (1)
    outb(0x1F3, lba & 0xFF);                  // LBA bits 0-7
    outb(0x1F4, (lba >> 8) & 0xFF);           // LBA bits 8-15
    outb(0x1F5, (lba >> 16) & 0xFF);          // LBA bits 16-23
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); // LBA bits 24-27 + master drive
    outb(0x1F7, 0x20);                        // Command to read (0x20 is the read command)

    // Wait for the disk to finish reading
    wait_for_disk_ready();

    for (int s = 0; s < sector_count; ++s)
    {
        wait_for_disk_ready(); // Wait for DRQ = 1, BSY = 0

        // Read 512 bytes (256 words)
        for (int i = 0; i < 256; ++i)
        {
            uint16_t data = inw(0x1F0);
            buffer[s * 512 + i * 2] = data & 0xFF;
            buffer[s * 512 + i * 2 + 1] = data >> 8;
        }
    }

    return buffer;
}

void KERNEL_WriteSectors(uint32_t lba, uint32_t sector_count, const void* data)
{
    const uint8_t* buffer = (const uint8_t*)data;

    // Step 1: Select the drive (Primary master)
    wait_for_disk_ready();

    // Set up the parameters for writing a sector
    outb(0x1F2, sector_count);                // Number of sectors
    outb(0x1F3, lba & 0xFF);                  // LBA bits 0-7
    outb(0x1F4, (lba >> 8) & 0xFF);           // LBA bits 8-15
    outb(0x1F5, (lba >> 16) & 0xFF);          // LBA bits 16-23
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); // LBA bits 24-27 + master drive
    outb(0x1F7, 0x30);                        // Command to write (0x30 is the write command)

    for (int s = 0; s < sector_count; ++s)
    {
        wait_for_disk_ready(); // Wait for DRQ = 1, BSY = 0

        // Write 512 bytes (256 words)
        for (int i = 0; i < 256; ++i)
        {
            uint16_t word = buffer[s * 512 + i * 2] | (buffer[s * 512 + i * 2 + 1] << 8);
            outw(0x1F0, word);
        }
    }

    // Flush the cache
    outb(0x1F7, 0xE7); // 0xE7 is the cache flush command
    wait_for_disk_ready();
}

typedef struct
{
    uint8_t type_guid[16];
    uint8_t unique_guid[16];
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t attributes;
    uint16_t name[36]; // 36 UTF-16 characters, each 2 bytes (72 bytes for UTF-8 name)
} __attribute__((packed)) GPTPartition;

int parse_gpt_partitions(GPTHeader* header, GPTPartition** partitions)
{
    uint64_t entries_lba = header->partition_entries_lba;
    uint32_t num_entries = header->num_partition_entries;
    uint32_t entry_size = header->size_of_partition_entry;

    uint8_t* entries = KERNEL_ReadSectors(entries_lba, (num_entries * entry_size + 511) /
                                                           512); // Round up to next sector

    *partitions = (GPTPartition*)kmalloc(num_entries * sizeof(GPTPartition));
    int valid_partitions = 0;

    for (int i = 0; i < num_entries; ++i)
    {
        GPTPartitionEntry* entry = (GPTPartitionEntry*)(entries + i * entry_size);

        // If the entry's type GUID is all zeros, it's unused
        if (entry->type_guid[0] == 0)
        {
            continue;
        }
        // Populate the GPTPartition struct
        GPTPartition* partition = &(*partitions)[valid_partitions];

        kmemcpy(partition->type_guid, entry->type_guid, 16);
        kmemcpy(partition->unique_guid, entry->unique_guid, 16);
        partition->first_lba = entry->first_lba;
        partition->last_lba = entry->last_lba;
        partition->attributes = entry->attributes;

        // Convert partition name from UTF-16 to UTF-8
        for (int j = 0; j < 36 && entry->name[j] != 0; ++j)
        {
            partition->name[j] = (char)entry->name[j];
        }

        valid_partitions++;
    }

    return valid_partitions;
}

GPTPartition* KERNEL_ParseGPTHeader(uint32_t lba)
{
    GPTHeader* header = (GPTHeader*)KERNEL_ReadSectors(lba, 1);
    if (header != NULL)
    {
        GPTPartition* partitions = NULL;
        int num_partitions = parse_gpt_partitions(header, &partitions);

        if (num_partitions > 0)
        {
            // for (int i = 0; i < num_partitions; i++)
            // {
            //     // Now you have `partitions` with useful information to continue working on the
            //     filesystem
            //     // For example, you can pass them to another function to continue working with
            //     filesystems

            //     // Example: Let's just print the first partition's details (for demonstration)
            //     Print(L"Partition %d Details:\n", i+1);
            //     Print(L"  First LBA: %llu\n", partitions[i].first_lba);
            //     Print(L"  Last LBA: %llu\n", partitions[i].last_lba);
            //     Print(L"  Name: ");
            //     for (int i2 = 0; i2 < 32; i2++)
            //     {
            //         Print(L"%c", partitions[i].name[i2]);
            //     }
            //     Print(L"\n  Attributes: %llu\n", partitions[i].attributes);

            //     // Free the partitions when done
            // }
            return partitions;
        }
        else
        {
            // Print(L"No valid partitions found.\n");
        }
    }

    return;
}

static uint32_t fnv1a_hash(const char* str)
{
    uint32_t hash = 2166136261u; // FNV offset basis
    while (*str)
    {
        hash ^= (uint8_t)(*str++);
        hash *= 16777619u; // FNV prime
    }
    return hash;
}

static void init_list_head(list_head_t* list)
{
    list->next = list;
    list->prev = list;
}

static vfs_entry_t* vfs_find_child(vfs_entry_t* parent, const char* name)
{
    list_head_t* pos;
    uint64_t i = 0;
    for (pos = parent->children.next; pos != &parent->children; pos = pos->next)
    {
        i++;
        vfs_entry_t* child = container_of(pos, vfs_entry_t, sibling);
        if (strcmp(child->name, name) == 0)
        {
            return child;
        }
    }
    return NULL;
}

static void list_add_tail(list_head_t* new_node, list_head_t* head)
{
    new_node->prev = head->prev;
    new_node->next = head;
    head->prev->next = new_node;
    head->prev = new_node;
}

static int vfs_entry_has_children(vfs_entry_t* entry)
{
    return entry->children.next != &entry->children;
}

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

void vfs_add_child(vfs_entry_t* parent, vfs_entry_t* child)
{
    child->parent = parent;
    list_add_tail(&child->sibling, &parent->children);
}

void vfs_init()
{
    GPTPartition* partitions = KERNEL_ParseGPTHeader(1);
    // partition index 1 is the one we want (exfat)
    GPTPartition* filesystemPartition = &partitions[1];

    if (ext2_init(FILESYSTEM, KERNEL_ReadSectors, KERNEL_WriteSectors,
                  filesystemPartition->first_lba, filesystemPartition->last_lba) != 0)
    {
        // Print(L"Failed to Initialized Filesystem\n");
    }

    vfs_entry_init(ROOT, "");
    ROOT->inode_num = 2;

    *DEV = vfs_create_entry(ROOT, "dev", EXT2_FT_DIR);
    (*DEV)->children_loaded = 1;
}

void vfs_populate_directory(vfs_entry_t* dir)
{
    if (dir->type != EXT2_FT_DIR || dir->children_loaded)
        return;
    ext2_dirent_t* dirent;
    ext2_dirent_iter_t iter;
    ext2_dir_iter_start(FILESYSTEM, &iter, dir->inode_num);

    uint64_t i = 0;
    while (ext2_dir_iter_next(FILESYSTEM, &iter, &dirent) == 0)
    {
        i++;
        vfs_entry_t* entry = kmalloc(sizeof(vfs_entry_t));

        vfs_entry_init(entry, dirent->name);
        entry->type = dirent->file_type;
        entry->inode_num = dirent->inode;
        entry->parent = dir;
        entry->inode = kmalloc(sizeof(ext2_inode));
        // TODO: Make a better way for dynamicall setting callbacks, fornow there are 8
        entry->ops = kmalloc(sizeof(void*) * 8);
        vfs_add_child(dir, entry);
    }
    // LOG_VARIABLE(descriptor.open_file->ops[DEV_WRITE], "r15");
    ext2_dir_iter_end(&iter);
}

int vfs_find_entry(vfs_entry_t* current, vfs_entry_t** out, const char* path)
{
    if (!current || !path || !*path)
        return 1;

    // Copy and manipulate the path

    kernel_strcpy(PATH, path); // or kmemcpy
    char* path_ptr = PATH;

    // If path is absolute, start from ROOT
    if (path_ptr[0] == '/')
    {
        current = ROOT;
        path_ptr++;
    }

    while (*path_ptr)
    {
        /* Extract next component */
        char* next_slash = path_ptr;
        while (*next_slash && *next_slash != '/')
            next_slash++;

        char component[256];
        size_t len = next_slash - path_ptr;
        if (len >= sizeof(component))
            return 1; // Component too long

        kmemcpy(component, path_ptr, len);
        component[len] = '\0';

        /* Lazy-load if needed */
        if (!current->children_loaded)
        {
            vfs_populate_directory(current);
            current->children_loaded = 1;
        }

        /* Look for the child directory */
        vfs_entry_t* next = vfs_find_child(current, component);
        if (!next)
            return 1; // Not found or not a dir
        current = next;

        // Move to next component
        path_ptr = *next_slash ? next_slash + 1 : next_slash;
    }

    *out = current;
    return 0;
}

open_file_t* vfs_open_file(vfs_entry_t* entry)
{
    return fdm_open_file(entry);
}

vfs_entry_t* vfs_create_entry(vfs_entry_t* dir, const char* name, entry_type_t type)
{

    vfs_entry_t* entry = kmalloc(sizeof(vfs_entry_t));
    vfs_entry_init(entry, name);
    entry->type = type;
    entry->inode_num = -1;
    entry->parent = dir;
    entry->inode = kmalloc(sizeof(ext2_inode));
    // TODO: Make a better way for dynamicall setting callbacks, fornow there are 8
    entry->ops = kmalloc(sizeof(void*) * 8);
    vfs_add_child(dir, entry);

    return entry;
}