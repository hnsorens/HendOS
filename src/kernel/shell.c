/* terminalDriver.c */
#include <kernel/shell.h>

#include <boot/elfLoader.h>
#include <fs/filesystem.h>
#include <kstring.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <memory/memoryMap.h>
#include <memory/paging.h>

#define SHELL_SUCCESS 0
#define SHELL_FAILURE 1
#define SHELL_INVALID_PARAMETER 2
#define SHELL_ERROR_1 3

typedef uint8_t shell_status_t;

void shell_init(shell_t* shell, dev_file_t* dev)
{
    shell->currentDir = ROOT;
    shell->tty_dev = dev;
}

int shell_isSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

char* shell_copyToken(const char* start, int length)
{
    char* token = (char*)kmalloc(length + 1);
    for (int i = 0; i < length; ++i)
    {
        token[i] = start[i];
    }
    token[length] = '\0';
    return token;
}

arg_list_t shell_splitArgs(const char* input)
{
    arg_list_t result = {NULL, 0};
    int capacity = 4;
    result.args = (char**)kmalloc(sizeof(char*) * capacity);

    const char* p = input;
    while (*p)
    {
        // Skip whitespace
        while (shell_isSpace(*p))
            p++;
        if (*p == '\0')
            break;

        const char* start = p;
        char quote = 0;

        if (*p == '\'' || *p == '"')
        {
            // Quoted string
            quote = *p;
            start = ++p;
            while (*p && *p != quote)
                p++;
        }
        else
        {
            // Unquoted string
            while (*p && !shell_isSpace(*p))
                p++;
        }

        int length = p - start;
        if (length > 0)
        {
            if (result.count >= capacity)
            {
                capacity *= 2;
                result.args = (char**)krealloc(result.args, sizeof(char*) * capacity);
            }
            result.args[result.count++] = shell_copyToken(start, length);
        }

        if (quote && *p == quote)
            p++; // Skip closing quote
    }

    return result;
}

void free_args(arg_list_t list)
{
    for (int i = 0; i < list.count; ++i)
    {
        kfree(list.args[i]);
    }
    kfree(list.args);
}

int shell_execute(shell_t* shell, char* input)
{
    arg_list_t args = shell_splitArgs(input);

    if (strcmp(args.args[0], "echo") == 0 && args.count > 1) /* echo Command */
    {
        dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
        dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, input + 5, kernel_strlen(input + 5));
        return 1;
    }
    else if (strcmp(args.args[0], "cd") == 0) /* cd Command*/
    {
        if (args.count == 2)
        {

            /* Print a message if the directory doesnt exist */
            if (shell_cd(shell, args.args[1]) != 0)
            {
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Directory doesnt exist", 22);
            }
        }
        else
        {
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Too many arguments", 18);
        }
        return 1;
    }
    else if (strcmp(args.args[0], "ls") == 0)
    {
        shell_ls(shell);
        return 1;
    }
    else if (strcmp(args.args[0], "pwd") == 0)
    {
        shell_pwd(shell);
        return 1;
    }
    else if (strcmp(args.args[0], "mkdir") == 0)
    {
        if (args.count == 2)
        {
            shell_status_t status = shell_mkdir(shell, args.args[1]);
            if (status == SHELL_FAILURE)
            {
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Failed to create directory", 26);
            }
            else if (status == SHELL_INVALID_PARAMETER)
            {
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 0);
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Invalid directory name", 22);
            }
        }
        else
        {
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Too many arguments", 18);
        }
        return 1;
    }
    else if (strcmp(args.args[0], "rmdir") == 0)
    {
        if (args.count == 2)
        {
            shell_status_t status = shell_rmdir(shell, args.args[1]);
            if (status == SHELL_FAILURE)
            {
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Failed to delete directory", 26);
            }
            else if (status == SHELL_INVALID_PARAMETER)
            {
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Invalid directory name", 23);
            }
        }
        else
        {
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Too many arguments", 18);
        }
        return 1;
    }
    else if (strcmp(args.args[0], "touch") == 0)
    {
        if (args.count == 2)
        {
            shell_status_t status = shell_touch(shell, args.args[1]);
            if (status == SHELL_FAILURE)
            {
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Failed to create file", 21);
            }
            else if (status == SHELL_INVALID_PARAMETER)
            {
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Invalid file name", 17);
            }
        }
        else
        {
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Too many arguments", 18);
        }
        return 1;
    }
    else if (strcmp(args.args[0], "rm") == 0)
    {
        if (args.count == 2)
        {
            shell_status_t status = shell_rm(shell, args.args[1]);
            if (status == SHELL_FAILURE)
            {
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Failed to delete file", 21);
            }
            else if (status == SHELL_INVALID_PARAMETER)
            {
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Invalid file name", 17);
            }
        }
        else
        {
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Too many arguments", 18);
        }
        return 1;
    }
    else if (strcmp(args.args[0], "cat") == 0)
    {
        if (args.count == 2)
        {
            shell_status_t status = shell_cat(shell, args.args[1]);
            if (status == SHELL_FAILURE)
            {
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Failed to concatinate file", 26);
            }
            else if (status == SHELL_INVALID_PARAMETER)
            {
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Invalid file name", 18);
            }
            else if (status == SHELL_ERROR_1)
            {
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Failed to read file", 19);
            }
        }
        else
        {
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Too many arguments", 18);
        }
        return 1;
    }
    else if (strcmp(args.args[0], "pc") == 0)
    {
        char character[] = {'\n', '0'};
        character[1] += *PROCESS_COUNT;
        dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, character, 2);
        return 1;
    }
    else if (strcmp(args.args[0], "gui") == 0)
    {
        fbcon_disable();
        shell_run(shell, "gui");
        return 0;
    }
    else
    {
        dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
        shell_status_t status = shell_run(shell, args.args[0]);
        if (status == SHELL_FAILURE)
        {
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Failed to concatinate file", 26);
        }
        else if (status == SHELL_INVALID_PARAMETER)
        {
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Invalid file name", 17);
        }
        else if (status == SHELL_ERROR_1)
        {
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "Failed to read file", 19);
        }
        else
        {
            return 0;
        }
        return 1;
    }
}

int shell_run(shell_t* shell, char* filename)
{
    directory_t* parent_directory;
    if (filesystem_findDirectory(ROOT, &parent_directory, "bin"))
    {
        return 1;
    }
    for (int i = 0; i < parent_directory->entry_count; i++)
    {
        if (parent_directory->entries[i]->file_type == EXT2_FT_REG_FILE &&
            kernel_strcmp(parent_directory->entries[i]->file.name, filename) == 0)
        {
            page_table_t* table = pageTable_createPageTable();
            return elfLoader_load(table, shell, &parent_directory->entries[i]->file.file);
        }
    }

    return 1;
}

int shell_cd(shell_t* shell, char* dirname)
{
    directory_t* out;
    if (filesystem_findDirectory(shell->currentDir, &out, dirname) == 0)
    {
        shell->currentDir = out;
        return 0;
    }
    return 1;
}

void shell_pwd(shell_t* shell)
{
    if (shell->currentDir == ROOT)
    {
        /* Prints / for root because path for root is NULL */
        dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
        dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "/", 1);
    }
    else
    {
        /* Prints Path */
        dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
        dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, shell->currentDir->path,
                      kernel_strlen(shell->currentDir->path));
    }
}

void shell_ls(shell_t* shell)
{
    directory_t* dir = shell->currentDir;

    /* Checks if the directory exists */
    uint8_t exists = 0;
    for (int i = 0; i < dir->entry_count; i++)
    {
        /* Print the name of the file/directory */
        if (dir->entries[i]->file_type == EXT2_FT_REG_FILE)
        {
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, dir->entries[i]->file.name,
                          kernel_strlen(dir->entries[i]->file.name));
        }
        else if (dir->entries[i]->file_type == EXT2_FT_DIR)
        {
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, dir->entries[i]->dir.name,
                          kernel_strlen(dir->entries[i]->dir.name));
        }
        else if (dir->entries[i]->file_type == EXT2_FT_CHRDEV)
        {
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, dir->entries[i]->dev_file.name,
                          kernel_strlen(dir->entries[i]->dev_file.name));
        }
    }
}

int shell_mkdir(shell_t* shell, char* directoryname)
{
    directory_t* parent_directory;
    if (filesystem_findParentDirectory(shell->currentDir, &parent_directory, directoryname) != 0)
    {
        return 1;
    }
    int i = kernel_strlen(directoryname);
    for (; i >= 0; --i)
    {
        if (directoryname[i] == '/')
        {
            directoryname = &directoryname[i + 1];
            break;
        }
    }
    char* path = kmalloc(kernel_strlen(parent_directory->path) + kernel_strlen(directoryname) + 5);
    path[0] = 0;
    kernel_strcat(path, parent_directory->path);
    kernel_strcat(path, "/");
    kernel_strcat(path, directoryname);
    if (!filesystem_validDirectoryname(directoryname))
    {
        return 2;
    }
    if (ext2_dir_create(FILESYSTEM, path, 0755) == 0)
    {
        filesystem_entry_t* entry = kmalloc(sizeof(filesystem_entry_t));
        entry->file_type = EXT2_FT_DIR;
        entry->dir = filesystem_createDir(parent_directory, directoryname);
        if (parent_directory->entry_count == 0)
        {
            parent_directory->entry_count++;
            parent_directory->entries = kmalloc(sizeof(filesystem_entry_t*));
        }
        else
        {
            parent_directory->entry_count++;
            parent_directory->entries =
                krealloc(parent_directory->entries, sizeof(void*) * parent_directory->entry_count);
        }
        parent_directory->entries[parent_directory->entry_count - 1] = entry;
    }
    else
    {
        return 1;
    }
    return 0;
}

int shell_rmdir(shell_t* shell, const char* directoryname)
{
    directory_t* parent_directory;
    if (filesystem_findParentDirectory(shell->currentDir, &parent_directory, directoryname) != 0)
    {
        return 1;
    }
    int i = kernel_strlen(directoryname);
    for (; i >= 0; --i)
    {
        if (directoryname[i] == '/')
        {
            directoryname = &directoryname[i + 1];
            break;
        }
    }
    char* path = kmalloc(kernel_strlen(parent_directory->path) + kernel_strlen(directoryname) + 2);
    path[0] = 0;
    kernel_strcat(path, parent_directory->path);
    kernel_strcat(path, "/");
    kernel_strcat(path, directoryname);
    if (!filesystem_validDirectoryname(directoryname))
    {
        return 2;
    }
    if (ext2_dir_delete(FILESYSTEM, path) == 0)
    {
        for (i = 0; i < parent_directory->entry_count; i++)
        {
            if (parent_directory->entries[i]->file_type == EXT2_FT_DIR &&
                kernel_strcmp(directoryname, parent_directory->entries[i]->dir.name) == 0)
            {
                kfree(parent_directory->entries[i]);
                parent_directory->entries[i] =
                    parent_directory->entries[--parent_directory->entry_count];
            }
        }
    }
    else
    {
        return 1;
    }
    return 0;
}

int shell_touch(shell_t* shell, const char* filename)
{
    directory_t* parent_directory;
    if (filesystem_findParentDirectory(shell->currentDir, &parent_directory, filename) != 0)
    {
        return 1;
    }
    int i = kernel_strlen(filename);
    for (; i >= 0; --i)
    {
        if (filename[i] == '/')
        {
            filename = &filename[i + 1];
            break;
        }
    }
    char* path = kmalloc(kernel_strlen(parent_directory->path) + kernel_strlen(filename) + 2);
    path[0] = 0;
    kernel_strcat(path, parent_directory->path);
    kernel_strcat(path, "/");
    kernel_strcat(path, filename);
    if (!filesystem_validFilename(filename))
    {
        return 2;
    }
    if (ext2_file_create(FILESYSTEM, path, 0644) == 0)
    {
        filesystem_entry_t* entry = kmalloc(sizeof(filesystem_entry_t));
        entry->file_type = EXT2_FT_REG_FILE;
        entry->file = filesystem_createFile(parent_directory, filename);
        if (parent_directory->entry_count == 0)
        {
            parent_directory->entry_count++;
            parent_directory->entries = kmalloc(sizeof(filesystem_entry_t*));
        }
        else
        {
            parent_directory->entry_count++;
            parent_directory->entries =
                krealloc(parent_directory->entries, sizeof(void*) * parent_directory->entry_count);
        }
        parent_directory->entries[parent_directory->entry_count - 1] = entry;
    }
    else
    {
        return 1;
    }
    return 0;
}

int shell_rm(shell_t* shell, const char* filename)
{
    directory_t* parent_directory;
    if (filesystem_findParentDirectory(shell->currentDir, &parent_directory, filename) != 0)
    {
        return 1;
    }
    int i = kernel_strlen(filename);
    for (; i >= 0; --i)
    {
        if (filename[i] == '/')
        {
            filename = &filename[i + 1];
            break;
        }
    }
    char* path = kmalloc(kernel_strlen(parent_directory->path) + kernel_strlen(filename) + 2);
    path[0] = 0;
    kernel_strcat(path, parent_directory->path);
    kernel_strcat(path, "/");
    kernel_strcat(path, filename);
    if (!filesystem_validFilename(filename))
    {
        return 2;
    }
    if (ext2_file_delete(FILESYSTEM, path) == 0)
    {
        for (int i = 0; i < parent_directory->entry_count; i++)
        {
            if (parent_directory->entries[i]->file_type == EXT2_FT_REG_FILE &&
                kernel_strcmp(filename, parent_directory->entries[i]->file.name) == 0)
            {
                kfree(parent_directory->entries[i]);
                parent_directory->entries[i] =
                    parent_directory->entries[--parent_directory->entry_count];
            }
        }
    }
    else
    {
        return 1;
    }
    return 0;
}

int shell_cat(shell_t* shell, char* filename)
{
    // dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "hello world\nhello world", 0);
    // return 0;
    directory_t* parent_directory;
    if (filesystem_findParentDirectory(shell->currentDir, &parent_directory, filename) != 0)
    {
        return 1;
    }
    for (int i = 0; i < parent_directory->entry_count; i++)
    {
        if (parent_directory->entries[i]->file_type == EXT2_FT_REG_FILE &&
            kernel_strcmp(parent_directory->entries[i]->file.name, filename) == 0)
        {
            ext2_file_t* file = &parent_directory->entries[i]->file.file;
            char buffer[512];
            uint64_t bytes_read;

            ext2_file_seek(file, 0, SEEK_SET);
            dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, "\n", 1);

            while (bytes_read = ext2_file_read(FILESYSTEM, file, buffer, sizeof(buffer) - 1))
            {
                if (bytes_read < 0)
                {
                    return 3;
                }

                buffer[bytes_read] = 0;
                dev_kernel_fn(shell->tty_dev->dev_id, DEV_WRITE, buffer, kernel_strlen(buffer));
            }
            return 0;
        }
    }
    return 1;
}