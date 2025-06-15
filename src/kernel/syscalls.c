/**
 * @file syscalls.c
 * @brief System Call Implementation
 *
 * Contains the kernel-side implementation of system calls.
 * These functions execute in kernel mode with full privileges.
 */

#include <boot/elfLoader.h>
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

#define SYSCALL_WRITE 4
#define SYSCALL_EXIT 1
#define SYSCALL_EXECVE 2
#define SYSCALL_INPUT 3
#define SYSCALL_CHDIR 5
#define SYSCALL_GETCWD 6
#define SYSCALL_MMAP 7

void sys_do_nothing() {}
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
    SYSCALLS[SYSCALL_WRITE] = sys_write;
    SYSCALLS[SYSCALL_INPUT] = sys_input;
    SYSCALLS[SYSCALL_EXIT] = sys_exit;
    SYSCALLS[SYSCALL_EXECVE] = sys_execve;
    SYSCALLS[SYSCALL_CHDIR] = sys_chdir;
    SYSCALLS[SYSCALL_GETCWD] = sys_getcwd;
    SYSCALLS[SYSCALL_MMAP] = sys_mmap;
}

/* ================================== SYSCALL API ===================================== */

// Count how many args
#define _GET_ARG_COUNT(_1, _2, _3, _4, _5, _6, COUNT, ...) COUNT
#define COUNT_ARGS(...) _GET_ARG_COUNT(__VA_ARGS__, 6, 5, 4, 3, 2, 1)

// Glue helpers
#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a##b

// Main entry macro
#define SYSCALL_ARGS(...)                                                                          \
    uint64_t __VA_ARGS__;                                                                          \
    __asm__ volatile(CONCAT(HANDLE_ASM_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)::CONCAT(            \
                         HANDLE_ARG_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)                        \
                     : CONCAT(HANDLE_COBB_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__));

#define HANDLE_ASM(x, register) "mov %%" register ", %0\n\t"

// Define handlers
#define ARG_ASM_1(x) HANDLE_ASM(x, "rdi")
#define ARG_ASM_2(x) HANDLE_ASM(x, "rsi")
#define ARG_ASM_3(x) HANDLE_ASM(x, "rdx")
#define ARG_ASM_4(x) HANDLE_ASM(x, "r10")
#define ARG_ASM_5(x) HANDLE_ASM(x, "r8")
#define ARG_ASM_6(x) HANDLE_ASM(x, "r9")

// Per-arity expansion
#define HANDLE_ASM_1(a1) ARG_ASM_1(a1)

#define HANDLE_ASM_2(a1, a2)                                                                       \
    ARG_ASM_1(a1)                                                                                  \
    ARG_ASM_2(a2)

#define HANDLE_ASM_3(a1, a2, a3)                                                                   \
    ARG_ASM_1(a1)                                                                                  \
    ARG_ASM_2(a2)                                                                                  \
    ARG_ASM_3(a3)

#define HANDLE_ASM_4(a1, a2, a3, a4)                                                               \
    ARG_ASM_1(a1)                                                                                  \
    ARG_ASM_2(a2)                                                                                  \
    ARG_ASM_3(a3)                                                                                  \
    ARG_ASM_4(a4)

#define HANDLE_ASM_5(a1, a2, a3, a4, a5)                                                           \
    ARG_ASM_1(a1)                                                                                  \
    ARG_ASM_2(a2)                                                                                  \
    ARG_ASM_3(a3)                                                                                  \
    ARG_ASM_4(a4)                                                                                  \
    ARG_ASM_5(a5)

#define HANDLE_ASM_6(a1, a2, a3, a4, a5, a6)                                                       \
    ARG_ASM_1(a1)                                                                                  \
    ARG_ASM_2(a2)                                                                                  \
    ARG_ASM_3(a3)                                                                                  \
    ARG_ASM_4(a4)                                                                                  \
    ARG_ASM_5(a5)                                                                                  \
    ARG_ASM_6(a6)

// Define handlers
#define ARG_COBB_1(x) "rdi",
#define ARG_COBB_2(x) "rsi",
#define ARG_COBB_3(x) "rdx",
#define ARG_COBB_4(x) "r10",
#define ARG_COBB_5(x) "r8",
#define ARG_COBB_6(x) "r9",

#define ARG_COBB_1_END(x) "rdi"
#define ARG_COBB_2_END(x) "rsi"
#define ARG_COBB_3_END(x) "rdx"
#define ARG_COBB_4_END(x) "r10"
#define ARG_COBB_5_END(x) "r8"
#define ARG_COBB_6_END(x) "r9"

// Per-arity expansion
#define HANDLE_COBB_1(a1) ARG_COBB_1_END(a1)

#define HANDLE_COBB_2(a1, a2)                                                                      \
    ARG_COBB_1(a1)                                                                                 \
    ARG_COBB_2_END(a2)

#define HANDLE_COBB_3(a1, a2, a3)                                                                  \
    ARG_COBB_1(a1)                                                                                 \
    ARG_COBB_2(a2)                                                                                 \
    ARG_COBB_3_END(a3)

#define HANDLE_COBB_4(a1, a2, a3, a4)                                                              \
    ARG_COBB_1(a1)                                                                                 \
    ARG_COBB_2(a2)                                                                                 \
    ARG_COBB_3(a3)                                                                                 \
    ARG_COBB_4_END(a4)

#define HANDLE_COBB_5(a1, a2, a3, a4, a5)                                                          \
    ARG_COBB_1(a1)                                                                                 \
    ARG_COBB_2(a2)                                                                                 \
    ARG_COBB_3(a3)                                                                                 \
    ARG_COBB_4(a4)                                                                                 \
    ARG_COBB_5_END(a5)

#define HANDLE_COBB_6(a1, a2, a3, a4, a5, a6)                                                      \
    ARG_COBB_1(a1)                                                                                 \
    ARG_COBB_2(a2)                                                                                 \
    ARG_COBB_3(a3)                                                                                 \
    ARG_COBB_4(a4)                                                                                 \
    ARG_COBB_5(a5)                                                                                 \
    ARG_COBB_6_END(a6)

// Define handlers
#define ARG_1(x) "=r"(x),
#define ARG_2(x) "=r"(x),
#define ARG_3(x) "=r"(x),
#define ARG_4(x) "=r"(x),
#define ARG_5(x) "=r"(x),
#define ARG_6(x) "=r"(x),

#define ARG_1_END(x) "=r"(x)
#define ARG_2_END(x) "=r"(x)
#define ARG_3_END(x) "=r"(x)
#define ARG_4_END(x) "=r"(x)
#define ARG_5_END(x) "=r"(x)
#define ARG_6_END(x) "=r"(x)

// Per-arity expansion
#define HANDLE_ARG_1(a1) ARG_1_END(a1)

#define HANDLE_ARG_2(a1, a2)                                                                       \
    ARG_1(a1)                                                                                      \
    ARG_2_END(a2)

#define HANDLE_ARG_3(a1, a2, a3)                                                                   \
    ARG_1(a1)                                                                                      \
    ARG_2(a2)                                                                                      \
    ARG_3_END(a3)

#define HANDLE_ARG_4(a1, a2, a3, a4)                                                               \
    ARG_1(a1)                                                                                      \
    ARG_2(a2)                                                                                      \
    ARG_3(a3)                                                                                      \
    ARG_4_END(a4)

#define HANDLE_ARG_5(a1, a2, a3, a4, a5)                                                           \
    ARG_1(a1)                                                                                      \
    ARG_2(a2)                                                                                      \
    ARG_3(a3)                                                                                      \
    ARG_4(a4)                                                                                      \
    ARG_5_END(a5)

#define HANDLE_ARG_6(a1, a2, a3, a4, a5, a6)                                                       \
    ARG_1(a1)                                                                                      \
    ARG_2(a2)                                                                                      \
    ARG_3(a3)                                                                                      \
    ARG_4(a4)                                                                                      \
    ARG_5(a5)                                                                                      \
    ARG_6_END(a6)

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
    uint64_t status;
    __asm__ volatile("mov %%rdi, %0\n\t" : "=r"(status)::"rdi");

    /* Add process memory to free stack */
    PROCESS_MEM_FREE_STACK[++PROCESS_MEM_FREE_STACK[0]] = (*CURRENT_PROCESS)->pid;

    /* Schedule next process and get new current */
    (*CURRENT_PROCESS) = schedule_end((*CURRENT_PROCESS));

    /* Prepare for context switch:
     * R12 = new process's page table root (CR3)
     * R11 = new process's stack pointer */
    __asm__ volatile("mov %0, %%r12\n\t" ::"r"((*CURRENT_PROCESS)->page_table->pml4) :);
    __asm__ volatile("mov %0, %%r11\n\t" ::"r"(&(*CURRENT_PROCESS)->process_stack_signature) :);
    TSS->ist1 =
        (uint64_t)(&(*CURRENT_PROCESS)->process_stack_signature) + sizeof(process_stack_layout_t);

    /* Terminal and display cleanup */
    // tty_endProcess(FBCON_TTY); /* Release terminal */
    // fbcon_update();            /* Update framebuffer */
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
    uint64_t addr, length, prot, flags, fd, offset;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     : "=r"(addr), "=r"(length)::"rdi", "rsi");
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
        pageTable_addPage(current->page_table, current->heap_end, (uint64_t)page / PAGE_SIZE_4KB, 1,
                          PAGE_SIZE_4KB, 4);
        pageTable_addPage(KERNEL_PAGE_TABLE, process_kernel_address(current->heap_end),
                          (uint64_t)page / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB, 0);
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
    uint64_t out, msg, len;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     "mov %%rdx, %2\n\t"
                     : "=r"(out), "=r"(msg), "=r"(len)::"rdi", "rsi", "rdx");

    if (out == 1) /* stdout */
    {
        /* Calculate proper virtual address offset for process memory */
        dev_kernel_fn(VCONS[0].dev_id, DEV_WRITE,
                      (ADDRESS_SECTION_SIZE * (2 + (*CURRENT_PROCESS)->pid)) + (char*)msg, len);
        /* Write to terminal output stream */
    }
    /* TODO: Implement stderr (FD 2) and other file descriptors */
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
                     : "=r"(in), "=r"(msg), "=r"(len)::"rdi", "rsi", "rdx");

    if (in == 1) /* stdout */
    {
        /* Calculate proper virtual address offset for process memory */

        dev_kernel_fn(VCONS[0].dev_id, DEV_READ, process_kernel_address(msg), len);
    }
    /* TODO: Implement stderr (FD 2) and other file descriptors */
}

void sys_execve()
{
    // SYSCALL_ARGS(name);
    uint64_t name, argc, argv;
    __asm__ volatile("mov %%rdi, %0\n\t"
                     "mov %%rsi, %1\n\t"
                     "mov %%rdx, %2\n\t"
                     : "=r"(name), "=r"(argc), "=r"(argv)::"rdi", "rsi", "rdx");

    directory_t* directory;
    filesystem_findDirectory(ROOT, &directory, "bin");

    for (int i = 0; i < directory->entry_count; i++)
    {
        filesystem_entry_t* entry = directory->entries[i];
        if (entry->file_type == EXT2_FT_REG_FILE &&
            kernel_strcmp(entry->file.name, process_kernel_address(name)) == 0)
        {
            page_table_t* table = pageTable_createPageTable();

            char** kernel_argv = kmalloc(sizeof(char*) * argc);

            for (int i = 0; i < argc; i++)
            {
                kernel_argv[i] = process_kernel_address(((char**)process_kernel_address(argv))[i]);
            }
            elfLoader_load(table, 0, &entry->file.file, argc, kernel_argv, 0, 0);
            kfree(kernel_argv);
        }
    }
}

void sys_open() {}

void sys_close() {}

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
    // SYSCALL_ARGS(directory_name);
    uint64_t directory_name;
    __asm__ volatile("mov %%rdi, %0\n\t" : "=r"(directory_name)::"rdi");

    directory_t* parent_directory;
    if (filesystem_findParentDirectory((*CURRENT_PROCESS)->cwd, &parent_directory,
                                       directory_name) != 0)
    {
        return 1;
    }
    int i = kernel_strlen(directory_name);
    for (; i >= 0; --i)
    {
        if (((char*)directory_name)[i] == '/')
        {
            directory_name = &((char*)directory_name)[i + 1];
            break;
        }
    }
    char* path = kmalloc(kernel_strlen(parent_directory->path) + kernel_strlen(directory_name) + 5);
    path[0] = 0;
    kernel_strcat(path, parent_directory->path);
    kernel_strcat(path, "/");
    kernel_strcat(path, directory_name);
    if (!filesystem_validDirectoryname(directory_name))
    {
        return 2;
    }
    if (ext2_dir_create(FILESYSTEM, path, 0755) == 0)
    {
        filesystem_entry_t* entry = kmalloc(sizeof(filesystem_entry_t));
        entry->file_type = EXT2_FT_DIR;
        entry->dir = filesystem_createDir(parent_directory, directory_name);
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
        // return 1;
    }
    // return 0;
}

void sys_rmdir()
{
    // SYSCALL_ARGS(directory_name);
    uint64_t directory_name;
    __asm__ volatile("mov %%rdi, %0\n\t" : "=r"(directory_name)::"rdi");

    directory_t* parent_directory;
    if (filesystem_findParentDirectory((*CURRENT_PROCESS)->cwd, &parent_directory,
                                       (char*)directory_name) != 0)
    {
        return 1;
    }
    int i = kernel_strlen((char*)directory_name);
    for (; i >= 0; --i)
    {
        if (((char*)directory_name)[i] == '/')
        {
            directory_name = &((char*)directory_name)[i + 1];
            break;
        }
    }
    char* path = kmalloc(kernel_strlen(parent_directory->path) + kernel_strlen(directory_name) + 2);
    path[0] = 0;
    kernel_strcat(path, parent_directory->path);
    kernel_strcat(path, "/");
    kernel_strcat(path, directory_name);
    if (!filesystem_validDirectoryname(directory_name))
    {
        return 2;
    }
    if (ext2_dir_delete(FILESYSTEM, path) == 0)
    {
        for (i = 0; i < parent_directory->entry_count; i++)
        {
            if (parent_directory->entries[i]->file_type == EXT2_FT_DIR &&
                kernel_strcmp(directory_name, parent_directory->entries[i]->dir.name) == 0)
            {
                kfree(parent_directory->entries[i]);
                parent_directory->entries[i] =
                    parent_directory->entries[--parent_directory->entry_count];
            }
        }
    }
    else
    {
        // return 1;
    }
    // return 0;
}

void sys_chdir()
{
    // SYSCALL_ARGS(buffer);
    uint64_t buffer;
    __asm__ volatile("mov %%rdi, %0\n\t" : "=r"(buffer)::"rdi");

    directory_t* out;
    if (filesystem_findDirectory((*CURRENT_PROCESS)->cwd, &out, process_kernel_address(buffer)) ==
        0)
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
                     : "=r"(buffer), "=r"(size)::"rdi", "rsi");

    size_t num_copied = min(strlen((*CURRENT_PROCESS)->cwd->path) + 1, size - 1);

    char* user_buffer = process_kernel_address(buffer);
    kmemcpy(user_buffer, (*CURRENT_PROCESS)->cwd->path, num_copied);
    user_buffer[num_copied] = 0;
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
