/* filesystem.c */
#include <arch/io.h>
#include <drivers/ext2.h>
#include <fs/filesystem.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>

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

void filesystem_populateDirectory(directory_t* dir);

file_t filesystem_createFile(directory_t* dir, const char* filename)
{
    file_t file;
    char* path = kmalloc(strlen(dir->path) + strlen(filename) + 2);
    path[0] = 0;
    kernel_strcat(path, dir->path);
    kernel_strcat(path, "/");
    kernel_strcat(path, filename);
    ext2_file_open(FILESYSTEM, &file.file, path);
    file.dir = dir;
    file.name = path + strlen(dir->path) + 1;
    return file;
}

directory_t filesystem_createDir(directory_t* dir, const char* directoryname)
{
    directory_t directory;
    char* path = kmalloc(strlen(dir->path) + strlen(directoryname) + 2);
    path[0] = 0;
    kernel_strcat(path, dir->path);
    kernel_strcat(path, "/");
    kernel_strcat(path, directoryname);
    directory.last = dir;
    directory.name = path + strlen(dir->path) + 1;
    directory.path = path;
    directory.entry_count = 0;
    directory.entries = 0;
    // filesystem_populateDirectory(&directory);
    return directory;
}

int filesystem_findParentDirectory(directory_t* current_dir, directory_t** out, char* dirname)
{
    directory_t* parent = current_dir;
    int i = kernel_strlen(dirname) - 1;
    for (; i >= 0; --i)
    {
        if (dirname[i] == '/')
        {
            dirname[i] = 0;
            if (filesystem_findDirectory(current_dir, &parent, dirname) != 0)
            {
                dirname[i] = '/';
                return 1;
            }
            dirname[i] = '/';
            break;
        }
    }
    *out = parent;
    return 0;
}

int filesystem_findDirectory(directory_t* current_dir, directory_t** out, char* dirname)
{
    if (*dirname == 0)
    {

        *out = current_dir;
        return 0;
    }

    /* Exact Path */
    if (dirname[0] == '/')
    {
        dirname++;
        return filesystem_findDirectory(ROOT, out, dirname);
    }

    /* Truncates the string, so it can be used as the directory name */
    uint32_t index = 0;
    uint8_t cont = 1;
    while (dirname[index] != 0 && dirname[index] != '/')
    {
        index++;
    }
    if (dirname[index] == 0)
        cont = 0;
    else
        dirname[index] = 0;

    directory_t* dir = current_dir;

    /* Checks if the directory exists */
    uint8_t exists = 0;
    for (int i = 0; i < dir->entry_count; i++)
    {
        if (dir->entries[i]->file_type == EXT2_FT_DIR &&
            kernel_strcmp(dir->entries[i]->dir.name, dirname) == 0)
        {
            *out = &dir->entries[i]->dir;
            filesystem_populateDirectory(*out);
            exists = 1;
        }
    }

    /* Checks for last directory or current directory */
    if (kernel_strcmp(dirname, ".") == 0)
    {
        exists = 1;
    }
    else if (kernel_strcmp(dirname, "..") == 0)
    {
        *out = current_dir->last;
        exists = 1;
    }

    /* If the directory doesnt exist, return before continuing*/
    if (!exists)
        return 1;

    /* Attempt to coninue */
    directory_t* current = *out;
    if (cont)
    {
        dirname[index] = '/';
        return filesystem_findDirectory(current, out, dirname + index + 1);
    }

    return 0;
}

void filesystem_populateDirectory(directory_t* dir)
{
    if (dir->entries)
        return;
    ext2_dirent_t* dirent;
    ext2_dir_iter_start(FILESYSTEM, &dir->iter, dir->path);
    dir->entries = kmalloc(sizeof(void*) * ext2_dir_count_entries(FILESYSTEM, dir->iter.inode));
    dir->entry_count = 0;
    while (ext2_dir_iter_next(FILESYSTEM, &dir->iter, &dirent) == 0)
    {
        if (strcmp(dirent->name, ".") == 0 || strcmp(dirent->name, "..") == 0)
            continue;
        filesystem_entry_t* entry = kmalloc(sizeof(filesystem_entry_t));
        entry->file_type = dirent->file_type;
        switch (dirent->file_type)
        {
        case EXT2_FT_REG_FILE:
            entry->file = filesystem_createFile(dir, dirent->name);
            break;
        case EXT2_FT_DIR:
            entry->dir = filesystem_createDir(dir, dirent->name);
            break;
        }
        dir->entries[dir->entry_count++] = entry;
    }
    ext2_dir_iter_end(&dir->iter);
}

dev_file_t* filesystem_createDevFile(const char* filename, uint64_t dev_flags)
{
    filesystem_entry_t* entry = kmalloc(sizeof(filesystem_entry_t));
    dev_file_t dev;
    char* path = kmalloc(6 + kernel_strlen(filename));
    path[0] = 0;
    kernel_strcat(path, "/dev/");
    kernel_strcat(path, filename);
    dev.dir = *DEV;
    dev.path = path;
    dev.name = path + 5;
    dev.dev_id = dev_create(dev.path, dev_flags);

    entry->file_type = EXT2_FT_CHRDEV;
    entry->dev_file = dev;

    if ((*DEV)->entry_count == 0)
    {
        (*DEV)->entry_count++;
        (*DEV)->entries = kmalloc(sizeof(filesystem_entry_t*));
    }
    else
    {
        (*DEV)->entry_count++;
        (*DEV)->entries =
            krealloc((*DEV)->entries, (*DEV)->entry_count * sizeof(filesystem_entry_t*));
    }
    (*DEV)->entries[(*DEV)->entry_count - 1] = entry;

    return &(*DEV)->entries[(*DEV)->entry_count - 1]->dev_file;
}

void filesystem_init()
{
    GPTPartition* partitions = KERNEL_ParseGPTHeader(1);
    // partition index 1 is the one we want (exfat)
    GPTPartition* filesystemPartition = &partitions[1];

    if (ext2_init(FILESYSTEM, KERNEL_ReadSectors, KERNEL_WriteSectors,
                  filesystemPartition->first_lba, filesystemPartition->last_lba) != 0)
    {
        // Print(L"Failed to Initialized Filesystem\n");
    }

    ROOT->path = "";
    ROOT->name = "";

    filesystem_populateDirectory(ROOT);

    /* Set Global Directories */
    for (int i = 0; i < ROOT->entry_count; i++)
    {
        if (ROOT->entries[i]->file_type == EXT2_FT_DIR &&
            kernel_strcmp(ROOT->entries[i]->dir.name, "dev") == 0)
        {
            *DEV = &ROOT->entries[i]->dir;
        }
    }

    // ext2_dir_iter_start(FILESYSTEM, &root.iter, "/");

    // if (ext2_dir_create(FILESYSTEM, "/mydir", 0755) != 0) {
    //     Print(L"Failed to create directory\n");
    // }

    // // Create a file
    // if (ext2_file_create(FILESYSTEM, "/mydir/test.txt", 0644) != 0) {
    //     Print(L"Failed to create file\n");
    // }

    // // Write to the file
    // ext2_file_t file;
    // if (ext2_file_open(FILESYSTEM, &file, "/mydir/test.txt") == 0) {
    //     const char* data = "Hello, EXT2 filesystem!";
    //     ext2_file_write(FILESYSTEM, &file, data, strlen(data));
    //     ext2_file_close(FILESYSTEM, &file);
    // }

    // // List directory contents
    // ext2_dirent_iter_t iter;
    // ext2_dirent_t* dirent;
    // if (ext2_dir_iter_start(FILESYSTEM, &iter, "/mydir") == 0) {
    //     Print(L"Contents of /mydir:\n");
    //     while (ext2_dir_iter_next(FILESYSTEM, &iter, &dirent) == 0) {
    //         Print(L"  %a\n", dirent->name);
    //     }
    //     ext2_dir_iter_end(&iter);
    // }
    // ext2_file_delete(FILESYSTEM, "/mydir/test.txt");

    // if (ext2_dir_iter_start(FILESYSTEM, &iter, "/mydir") == 0) {
    //     Print(L"Contents of /mydir:\n");
    //     while (ext2_dir_iter_next(FILESYSTEM, &iter, &dirent) == 0) {
    //         Print(L"  %a\n", dirent->name);
    //     }
    //     ext2_dir_iter_end(&iter);
    // }

    // if (ext2_dir_delete(FILESYSTEM, "/mydir") != 0)
    // {
    //     Print(L"Failed to delete directory");
    // }

    // if (ext2_dir_iter_start(FILESYSTEM, &iter, "/") == 0) {
    //     Print(L"Contents of /mydir:\n");
    //     while (ext2_dir_iter_next(FILESYSTEM, &iter, &dirent) == 0) {
    //         Print(L"  %a\n", dirent->name);
    //     }
    //     ext2_dir_iter_end(&iter);
    // }
}

uint8_t filesystem_validDirectoryname(char* dirname)
{
    if (kernel_strcmp(dirname, ""))
    {
        return 1;
    }
    return 0;
}

uint8_t filesystem_validFilename(char* filename)
{
    if (kernel_strcmp(filename, ""))
    {
        return 1;
    }
    return 0;
}