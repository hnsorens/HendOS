/**
 * @file syscalls.c
 * @brief System Call Implementation
 *
 * Contains the kernel-side implementation of system calls.
 * These functions execute in kernel mode with full privileges.
 */

#include <boot/elfLoader.h>
#include <fs/fdm.h>
#include <fs/vfs.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/syscalls.h>
#include <kmath.h>
#include <kstring.h>
#include <memory/kglobals.h>
#include <memory/memoryMap.h>
#include <memory/paging.h>
#include <misc/debug.h>

#define IA32_EFER 0xC0000080
#define IA32_STAR 0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_FMASK 0xC0000084

#define KERNEL_CS 0x08
#define USER_CS 0x1B

static uint64_t rdmsr(uint32_t msr)
{
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

/**
 * @brief Inline asm helpers for MSR read/write
 */
static inline void wrmsr(uint32_t msr, uint64_t value)
{
    uint32_t low = (uint32_t)(value & 0xFFFFFFFF);
    uint32_t high = (uint32_t)(value >> 32);
    __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high) : "memory");
}

/* Read CR4 register */
static inline uint64_t read_cr4()
{
    uint64_t val;
    __asm__ volatile("mov %%cr4, %0" : "=r"(val));
    return val;
}

/* Write CR4 register */
static inline void write_cr4(uint64_t val)
{
    __asm__ volatile("mov %0, %%cr4" : : "r"(val) : "memory");
}

/* ================================ Public API ============================= */

/**
 * @brief Initializes all syscalls
 */

extern void syscall_stub(void);

#define syscall_def(name)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
    void sys_##name();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
    SYSCALLS[++syscall_counter] = sys_##name;

#define def_syscall(name) SYSCALLS[SYSCALL_##name] sys_ % % name;

void sys_do_nothing()
{
    process_signal(*CURRENT_PROCESS, SIGSYS);
}

void syscall_init()
{
    // // Enable SYSCALL/SYSRET via EFER
    // uint64_t efer = rdmsr(IA32_EFER);
    // efer |= 1;
    // wrmsr(IA32_EFER, efer);

    // // Set STAR, LSTAR, FMASK as before
    // wrmsr(IA32_STAR, ((uint64_t)USER_CS << 48) | ((uint64_t)KERNEL_CS << 32));
    // wrmsr(IA32_LSTAR, (uint64_t)EXTERN(uint64_t, syscall_stub));
    // wrmsr(IA32_FMASK, (1 << 9)); // Mask IF (disable interrupts during syscall)

    /* Add all syscall functions */
    for (int i = 0; i < 512; i++)
    {
        SYSCALLS[i] = sys_do_nothing;
    }

    size_t syscall_counter = 0;
    syscall_def(exit);      /* SYSCALL 1 */
    syscall_def(execve);    /* SYSCALL 2 */
    syscall_def(input);     /* SYSCALL 3 */
    syscall_def(write);     /* SYSCALL 4 */
    syscall_def(chdir);     /* SYSCALL 5 */
    syscall_def(getcwd);    /* SYSCALL 6 */
    syscall_def(mmap);      /* SYSCALL 7 */
    syscall_def(fork);      /* SYSCALL 8 */
    syscall_def(execvp);    /* SYSCALL 9 */
    syscall_def(getpgid);   /* SYSCALL 10 */
    syscall_def(setpgid);   /* SYSCALL 11 */
    syscall_def(open);      /* SYSCALL 12 */
    syscall_def(dup2);      /* SYSCALL 13 */
    syscall_def(close);     /* SYSCALL 14 */
    syscall_def(tcsetpgrp); /* SYSCALL 15 */
    syscall_def(tcgetpgrp); /* SYSCALL 16 */
    syscall_def(waitpid);   /* SYSCALL 17 */
    syscall_def(setsid);    /* SYSCALL 18 */
    syscall_def(getsid);    /* SYSCALL 19 */
    syscall_def(kill);      /* SYSCALL 20 */
}

/* ================================== SYSCALL API ===================================== */

/**
 * @brief Process termination handler
 * @param current Pointer to current process control block
 * @param status Process exit status
 *
 * Detailed execution flow:
 * 1. Memory Management:
 *    - Adds process memory to free stack for reuse
 *    - PROCESS_MEM_FREE_STACK[0] contains stack pointer
 *    - PROCESS_MEM_FREE_STACK[1..N] contain free indices
 *
 * 2. Process Scheduling:
 *    - schedule_end() performs:
 *      a. Removes process from scheduler queue
 *      b. Selects next process to run
 *      c. Returns new current process pointer
 *
 * 3. Context Preparation:
 *    - Loads new process's page table into R12 (for context switch)
 *    - Loads new stack pointer into R11 (for context restore)
 *
 * 4. Cleanup:
 *    - Releases terminal control (tty_endProcess)
 *    - Updates framebuffer to reflect process change
 *
 * Note: Uses inline ASM to prepare registers for context_switch.S
 */
void sys_exit()
{

    // SYSCALL_ARGS(status);
    uint64_t exit_code = SYS_ARG_1(*CURRENT_PROCESS);

    process_exit(*CURRENT_PROCESS, exit_code << 8);
}

#define PROT_NONE 0  /* Pages cannot be accessed */
#define PROT_EXEC 1  /* Pages can be executed */
#define PROT_WRITE 2 /* Pages can be written */
#define PROT_READ 4  /* Pages can be read */

#define MAP_PRIVATE 0         /* Changes are shared with other processes */
#define MAP_SHARED 1          /* Changes are private to the process */
#define MAP_ANONYMOUS 2       /* Memory is not blocked by any file (raw memory) */
#define MAP_FIXED 4           /* Force mapping at addr. Dangerous can overwrite existing mapping */
#define MAP_FIXED_NOREPLACE 8 /* Map at addr, but fail it already mapped */
#define MAP_GROWSDOWN 16      /* Stack like mapping that grows downward */
#define MAP_NORESERVE 32      /* Don't reserve swap space. May fail on access */
#define MAP_POPULATE 64       /* Prefault all pages (no page faults later) */
#define MAP_LOCKED 128        /* Lock the memory so it doesn't get swapped */
#define MAP_HUGETLB 256       /* User huge pages */

/**
 * @brief Allocates memory for user space
 *
 * @param addr Optional starting address. Usually NULL to let the kernel decide
 * @param Size of the mapping (page aligned)
 * @param prot Memory protection flags
 * @param flags Mapping type and behavior flags
 * @param fd File descriptor if mapping a file
 * @param offset offset into the file (page aligned)
 */
void sys_mmap()
{

    // TODO: make syscall more robust with making sure registers dont get overwritten (along with
    // interrupts)
    uint64_t addr = SYS_ARG_1(*CURRENT_PROCESS);
    uint64_t length = SYS_ARG_2(*CURRENT_PROCESS);
    uint64_t prot, flags, fd, offset;
    /**
     * TODO: implement other flags
     *
     * Right now:
     * prot = PROT_READ | PROT_WRITE
     * flags = MAP_PRIVATE | MAP_ANONYMOUS
     */

    uint64_t page_count = length / 4096;
    for (int i = 0; i < page_count; i++)
    {
        void* page = pages_allocatePage(PAGE_SIZE_4KB);

        process_t* current = (*CURRENT_PROCESS);
        pageTable_addPage(&current->page_table, current->heap_end, (uint64_t)page / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB, 4);
        current->heap_end += PAGE_SIZE_4KB;
    }
}

/**
 * @brief Output writing implementation
 * @param out File descriptor (1=stdout)
 * @param msg User-space message pointer
 * @param len Message length in bytes
 *
 * Memory Safety:
 * - The (char*)msg is offset by ADDRESS_SECTION_SIZE*(2+index) because:
 *   - Each process has isolated memory sections
 *   - Section 0: Kernel
 *   - Section 1: Shared
 *   - Section 2+N: Per-process memory
 *   - This calculates the proper virtual address offset
 *
 * Output Handling:
 * - Only stdout (FD 1) is currently implemented
 * - Writes go to the process's controlling terminal
 * - Forces framebuffer update for immediate visibility
 */
void sys_write()
{
    uint64_t out = SYS_ARG_1(*CURRENT_PROCESS);
    uint64_t msg = SYS_ARG_2(*CURRENT_PROCESS);
    uint64_t len = SYS_ARG_3(*CURRENT_PROCESS);

    file_descriptor_t descriptor = (*CURRENT_PROCESS)->file_descriptor_table[out];
    uint64_t pgid = (*CURRENT_PROCESS)->pgid;

    if (descriptor.open_file->type == EXT2_FT_CHRDEV)
    {
        uint64_t pgrp = descriptor.open_file->ops[CHRDEV_GETGRP](0, 0);
        if (pgrp != (*CURRENT_PROCESS)->pgid)
        {
            process_signal(*CURRENT_PROCESS, SIGTTOU);
            return;
        }
    }

    // TODO: add this to the queue instead so it can also run on user processes
    descriptor.open_file->ops[DEV_WRITE](msg, len);
}

/**
 * @brief Terminal input implementation
 * @param out File descriptor (1=stdout)
 * @param msg User-space message pointer
 * @param len Message length in bytes
 */
void sys_input()
{
    // SYSCALL_ARGS(in, msg, len);
    uint64_t in, msg, len;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     "mov %%rdx, %2\n\t"
                     : "=r"(in), "=r"(msg), "=r"(len)
                     :
                     : "rdi", "rsi", "rdx");

    file_descriptor_t descriptor = (*CURRENT_PROCESS)->file_descriptor_table[in];
    uint64_t pgid = (*CURRENT_PROCESS)->pgid;

    if (descriptor.open_file->type == EXT2_FT_CHRDEV)
    {
        uint64_t pgrp = descriptor.open_file->ops[CHRDEV_GETGRP](0, 0);
        if (pgrp != (*CURRENT_PROCESS)->pgid)
        {
            process_signal(*CURRENT_PROCESS, SIGTTIN);
            return;
        }
    }

    // TODO: add this to the queue instead so it can also run on user processes
    descriptor.open_file->ops[DEV_READ](msg, len);

    /* TODO: Implement stderr (FD 2) and other file descriptors */
}

void sys_fork()
{
    process_fork();
}

void sys_execvp() {}

void sys_execve()
{
    // SYSCALL_ARGS(name);
    uint64_t name, argc, argv;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     "mov %%rdx, %2\n\t"
                     : "=r"(name), "=r"(argc), "=r"(argv)
                     :
                     : "rdi", "rsi", "rdx");
    vfs_entry_t* directory;
    vfs_find_entry(ROOT, &directory, "bin");
    vfs_entry_t* executable;
    vfs_find_entry(directory, &executable, name);
    char** kernel_argv = kmalloc(sizeof(char*) * argc);
    for (int i = 0; i < argc; i++)
    {
        kernel_argv[i] = ((char**)argv)[i];
    }
    process_execvp(vfs_open_file(executable), argc, kernel_argv, 0, 0);
    kfree(kernel_argv);
}

void sys_dup2()
{
    uint64_t old_fd, new_fd;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     : "=r"(old_fd), "=r"(new_fd)
                     :
                     : "rdi", "rsi");

    // TODO: When dev and normal files are combined, make the file descriptors ONLY file file_t*
    // then null if closed
    if (new_fd > (*CURRENT_PROCESS)->file_descriptor_capacity)
    {
        (*CURRENT_PROCESS)->file_descriptor_capacity = new_fd;
        (*CURRENT_PROCESS)->file_descriptor_table = krealloc((*CURRENT_PROCESS)->file_descriptor_table, sizeof(file_descriptor_t) * (*CURRENT_PROCESS)->file_descriptor_capacity);
    }

    (*CURRENT_PROCESS)->file_descriptor_table[new_fd] = (*CURRENT_PROCESS)->file_descriptor_table[old_fd];
}

void sys_open()
{
    uint64_t path, perms;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     : "=r"(path), "=r"(perms)
                     :
                     : "rdi", "rsi");
    // LOG_VARIABLE(descriptor.open_file->ops[DEV_WRITE], "r15");
    char* kernel_path = path;
    vfs_entry_t* entry;
    process_t* current = (*CURRENT_PROCESS);
    uint64_t file_descriptor = 0;

    if (vfs_find_entry(current->cwd, &entry, kernel_path) == 0)
    {
        if (current->file_descriptor_capacity == current->file_descriptor_count)
        {
            current->file_descriptor_capacity *= 2;
            current->file_descriptor_table = krealloc(current->file_descriptor_table, sizeof(file_descriptor_t) * current->file_descriptor_capacity);
        }
        file_descriptor_t descriptor = {};
        descriptor.flags = perms;
        descriptor.open_file = fdm_open_file(entry);

        // find a free spot
        file_descriptor = current->file_descriptor_count;
        current->file_descriptor_table[current->file_descriptor_count++] = descriptor;
    }
    current->process_stack_signature.rax = file_descriptor;
}

void sys_close()
{
    uint64_t fd;
    __asm__ volatile("mov %%rdi, %0\n\t" : "=r"(fd) : : "rdi");

    (*CURRENT_PROCESS)->file_descriptor_table[fd].flags = 0;
}

void sys_read() {}

// void sys_write() {} // File one

void sys_lseek() {}

void sys_pread() {}

void sys_pwrite() {}

void sys_unlink() {}

void sys_truncate() {}

void sys_ftruncate() {}

void sys_rename() {}

void sys_link() {}

void sys_symlink() {}

void sys_readlink() {}

void sys_mkdir()
{
    // // SYSCALL_ARGS(directory_name);
    // uint64_t directory_name;
    // __asm__ volatile("mov %%rdi, %0\n\t" : "=r"(directory_name): :"rdi");

    // directory_t* parent_directory;
    // if (filesystem_findParentDirectory((*CURRENT_PROCESS)->cwd, &parent_directory,
    //                                    directory_name) != 0)
    // {
    //     return 1;
    // }
    // int i = kernel_strlen(directory_name);
    // for (; i >= 0; --i)
    // {
    //     if (((char*)directory_name)[i] == '/')
    //     {
    //         directory_name = &((char*)directory_name)[i + 1];
    //         break;
    //     }
    // }
    // char* path = kmalloc(kernel_strlen(parent_directory->path) + kernel_strlen(directory_name) +
    // 5); path[0] = 0; kernel_strcat(path, parent_directory->path); kernel_strcat(path, "/");
    // kernel_strcat(path, directory_name);
    // if (!filesystem_validDirectoryname(directory_name))
    // {
    //     return 2;
    // }
    // if (ext2_dir_create(FILESYSTEM, path, 0755) == 0)
    // {
    //     filesystem_entry_t* entry = kmalloc(sizeof(filesystem_entry_t));
    //     entry->file_type = EXT2_FT_DIR;
    //     entry->dir = filesystem_createDir(parent_directory, directory_name);
    //     if (parent_directory->entry_count == 0)
    //     {
    //         parent_directory->entry_count++;
    //         parent_directory->entries = kmalloc(sizeof(filesystem_entry_t*));
    //     }
    //     else
    //     {
    //         parent_directory->entry_count++;
    //         parent_directory->entries =
    //             krealloc(parent_directory->entries, sizeof(void*) *
    //             parent_directory->entry_count);
    //     }
    //     parent_directory->entries[parent_directory->entry_count - 1] = entry;
    // }
    // else
    // {
    //     // return 1;
    // }
    // // return 0;
}

void sys_rmdir()
{
    // // SYSCALL_ARGS(directory_name);
    // uint64_t directory_name;
    // __asm__ volatile("mov %%rdi, %0\n\t" : "=r"(directory_name): :"rdi");

    // directory_t* parent_directory;
    // if (filesystem_findParentDirectory((*CURRENT_PROCESS)->cwd, &parent_directory,
    //                                    (char*)directory_name) != 0)
    // {
    //     return 1;
    // }
    // int i = kernel_strlen((char*)directory_name);
    // for (; i >= 0; --i)
    // {
    //     if (((char*)directory_name)[i] == '/')
    //     {
    //         directory_name = &((char*)directory_name)[i + 1];
    //         break;
    //     }
    // }
    // char* path = kmalloc(kernel_strlen(parent_directory->path) + kernel_strlen(directory_name) +
    // 2); path[0] = 0; kernel_strcat(path, parent_directory->path); kernel_strcat(path, "/");
    // kernel_strcat(path, directory_name);
    // if (!filesystem_validDirectoryname(directory_name))
    // {
    //     return 2;
    // }
    // if (ext2_dir_delete(FILESYSTEM, path) == 0)
    // {
    //     for (i = 0; i < parent_directory->entry_count; i++)
    //     {
    //         if (parent_directory->entries[i]->file_type == EXT2_FT_DIR &&
    //             kernel_strcmp(directory_name, parent_directory->entries[i]->dir.name) == 0)
    //         {
    //             kfree(parent_directory->entries[i]);
    //             parent_directory->entries[i] =
    //                 parent_directory->entries[--parent_directory->entry_count];
    //             break;
    //         }
    //     }
    // }
    // else
    // {
    //     // return 1;
    // }
    // // return 0;
}

void sys_chdir()
{
    // SYSCALL_ARGS(buffer);
    uint64_t buffer;
    __asm__ volatile("mov %%rdi, %0\n\t" : "=r"(buffer) : : "rdi");

    vfs_entry_t* out;
    if (vfs_find_entry((*CURRENT_PROCESS)->cwd, &out, buffer) == 0)
    {
        (*CURRENT_PROCESS)->cwd = out;
    }
}

void sys_getcwd()
{
    // SYSCALL_ARGS(buffer, size);
    uint64_t buffer, size;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     : "=r"(buffer), "=r"(size)
                     :
                     : "rdi", "rsi");

    // TODO: Generate path string

    char* user_buffer = buffer;
    user_buffer[0] = 0;
    uint64_t offset = 0;
    vfs_path((*CURRENT_PROCESS)->cwd, user_buffer, &offset);

    // kernel_strcat(&PATH[offset], (*CURRENT_PROCESS)->cwd->name);
    // offset += kernel_strlen((*CURRENT_PROCESS)->cwd->name);
    // PATH[offset] = '\0';
}

void sys_getdents() {}

void sys_stat() {}

void sys_fstat() {}

void sys_lstat() {}

void sys_chmod() {}

void sys_fchmod() {}

void sys_chown() {}

void sys_fchown() {}

void sys_fntl() {}

void sys_access() {}

/**
 * @brief Set a processes process group id
 */
void sys_setpgid()
{

    uint64_t pid, pgid;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     : "=r"(pid), "=r"(pgid)
                     :
                     : "rdi", "rsi");

    process_t* process;
    if (pid == 0)
    {
        process = *CURRENT_PROCESS;
    }
    else
    {
        process = pid_hash_lookup(PID_MAP, pid);
    }

    if (pgid == 0)
    {
        pgid = process->pid;
    }

    if (process->pgid != 0)
    {
        process_remove_from_group(process);
    }

    process_add_to_group(process, pgid);
}

/**
 * @brief Get a processes process group ID
 */
void sys_getpgid()
{
    uint64_t pid;
    __asm__ volatile("mov %%rdi, %0\n\t" : "=r"(pid) : : "rdi");

    if (pid == 0)
    {
        (*CURRENT_PROCESS)->process_stack_signature.rax = (*CURRENT_PROCESS)->pgid;
    }
    else
    {
        process_t* process = pid_hash_lookup(PID_MAP, pid);
        (*CURRENT_PROCESS)->process_stack_signature.rax = process->pid;
    }
}

/**
 * @brief Get the calling process's PGID
 */
void sys_getpgrp() {}

/**
 * @brief Set calling process's PGID to its PID
 */
void sys_setpgrp()
{
    uint64_t pid;
    __asm__ volatile("mov %%rdi, %0\n\t" : "=r"(pid) : : "rdi");

    process_t* process;

    if (pid == 0)
    {
        process = *CURRENT_PROCESS;
    }
    else
    {
        process = pid_hash_lookup(PID_MAP, pid);
    }

    if (process->pid == process->pgid)
        return;

    if (process->pgid != 0)
    {
        process_remove_from_group(process);
    }

    process_add_to_group(process, process->pid);
}

void sys_setsid()
{

    uint64_t pid, sid;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     : "=r"(pid), "=r"(sid)
                     :
                     : "rdi", "rsi");

    process_t* process;
    if (pid == 0)
    {
        process = *CURRENT_PROCESS;
    }
    else
    {
        process = pid_hash_lookup(PID_MAP, pid);
    }

    if (sid == 0)
    {
        sid = process->pid;
    }

    if (process->sid != 0)
    {
        process_remove_from_group(process);
    }

    process_add_to_group(process, sid);
}

/**
 * @brief Get a processes process group ID
 */
void sys_getsid()
{
    uint64_t pid;
    __asm__ volatile("mov %%rdi, %0\n\t" : "=r"(pid) : : "rdi");

    if (pid == 0)
    {
        (*CURRENT_PROCESS)->process_stack_signature.rax = (*CURRENT_PROCESS)->sid;
    }
    else
    {
        process_t* process = pid_hash_lookup(PID_MAP, pid);
        (*CURRENT_PROCESS)->process_stack_signature.rax = process->pid;
    }
}

/**
 * @brief Set foreground process group for terminal
 */
void sys_tcgetpgrp()
{
    uint64_t fd;
    __asm__ volatile("mov %%rdi, %0\n\t" : "=r"(fd) : : "rdi");

    file_descriptor_t descriptor = (*CURRENT_PROCESS)->file_descriptor_table[fd];
    open_file_t* open_file = descriptor.open_file;

    /* Make sure the device is a character device */
    if (open_file->type != EXT2_FT_CHRDEV)
    {
        return;
    }

    open_file->ops[CHRDEV_SETGRP](0, 0);
}

/**
 * @brief Get foreground process group for terminal
 */
void sys_tcsetpgrp()
{
    uint64_t fd, pgrp;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     : "=r"(fd), "=r"(pgrp)
                     :
                     : "rdi", "rsi");

    file_descriptor_t descriptor = (*CURRENT_PROCESS)->file_descriptor_table[fd];
    open_file_t* open_file = descriptor.open_file;

    /* Make sure the device is a character device */
    if (open_file->type != EXT2_FT_CHRDEV)
    {
        return;
    }

    if (pgrp == 0)
    {
        pgrp = (*CURRENT_PROCESS)->pgid;
    }

    open_file->ops[CHRDEV_SETGRP](pgrp, 0);
}

/**
 * @brief Wait for processes or process groups to exit
 */
void sys_waitpid()
{
    uint64_t pid, options;
    uint64_t* status;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     "mov %%rdx, %2\n\t"
                     : "=r"(pid), "=r"(status), "=r"(options)
                     :
                     : "rdi", "rsi", "rdx");

    process_t* process = pid_hash_lookup(PID_MAP, pid);

    if (process->flags & PROCESS_ZOMBIE)
    {
        *((uint64_t*)SYS_ARG_2(*CURRENT_PROCESS)) = process->status;
        (*CURRENT_PROCESS)->process_stack_signature.rax = process->pid;
        return;
    }

    process->waiting_parent_pid = (*CURRENT_PROCESS)->pid;

    schedule_block((*CURRENT_PROCESS));
    (*CURRENT_PROCESS) = scheduler_nextProcess(*CURRENT_PROCESS);

    /* Prepare for context switch:
     * R12 = new process's page table root (CR3)
     * R11 = new process's stack pointer */
    INTERRUPT_INFO->cr3 = (*CURRENT_PROCESS)->page_table;
    INTERRUPT_INFO->rsp = &(*CURRENT_PROCESS)->process_stack_signature;
    TSS->ist1 = (uint64_t)(*CURRENT_PROCESS) + sizeof(process_stack_layout_t);
}

void sys_kill()
{
    int64_t pid;
    uint64_t signal;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     : "=r"(pid), "=r"(signal)
                     :
                     : "rdi", "rsi");

    if (pid == -1)
    {
        process_signal_all(signal);
    }
    else if (pid < 0)
    {
        process_group_t* group = pid_hash_lookup(PGID_MAP, -pid);
        process_group_signal(group, signal);
    }
    else
    {
        process_t* process = pid_hash_lookup(PID_MAP, pid);
        process_signal(process, signal);
    }
}

void sys_seek()
{
    uint64_t fd = SYS_ARG_1(*CURRENT_PROCESS);
    uint64_t offset = SYS_ARG_2(*CURRENT_PROCESS);
    uint64_t whence = SYS_ARG_3(*CURRENT_PROCESS);

    file_descriptor_t descriptor = (*CURRENT_PROCESS)->file_descriptor_table[fd];
    uint64_t pgid = (*CURRENT_PROCESS)->pgid;

    if (descriptor.open_file->type == EXT2_FT_REG_FILE)
    {
        ext2_file_seek(descriptor.open_file, offset, whence);
    }
}