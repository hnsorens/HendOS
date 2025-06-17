/**
 * @file process.h
 *
 */

#ifndef PROCESS_H
#define PROCESS_H

#include <fs/filesystem.h>
#include <memory/pageTable.h>

#define DECLARE_PROCESS process_t* process = (*CURRENT_PROCESS)

#define PROCESS_BLOCKING 1

/**
 * @struct context stack layout
 */
typedef struct
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;

} __attribute__((packed)) process_stack_layout_t;

typedef struct process_group_t
{
    uint64_t pgid; /* pid of group leader */
    struct process_t** processes;
    uint64_t process_count;
    uint64_t process_capacity;
} process_group_t;

/**
 * @struct Process struct for containing context information
 */
typedef struct process_t
{
    process_stack_layout_t process_stack_signature; /* Stack Signature for switching contexts*/
    page_table_t* page_table;    /* Page table for process context, also used for kernel index */
    uint64_t pid;                /* Process ID */
    uint64_t ppid;               /* Parent Process ID*/
    uint64_t pgid;               /* Process Group ID */
    uint64_t sid;                /* Session ID */
    struct process_t* next;      /* next process context to switch to */
    struct process_t* last;      /* last process contexgt */
    uint64_t entry;              /* entry into process */
    uint64_t stackPointer;       /* saved stack pointer */
    uint64_t process_heap_ptr;   /* points to end of heap */
    uint64_t process_shared_ptr; /* points to end of shared memory */
    uint64_t flags;
    struct directory_t* cwd;
    void* heap_end;
    uint64_t kernel_memory_index;
} __attribute__((packed)) process_t;

/**
 * @enum Different types of pages that can be added to a process
 */
typedef enum process_page_use
{
    PROCESS_PAGE_HEAP,
    PROCESS_PAGE_SHARED,
} process_page_use;

/**
 * @brief Generates a new process id
 * @return Newly generated unique process id
 */
uint64_t process_genPID();

/**
 * @brief Translates memory address from user space to kernel space
 * @param addr address to be tranlated
 * @return kernel address
 */
uint64_t process_kernel_address(uint64_t addr);

/**
 * @brief Maps a page to the process
 * @param page_number start page that is being added
 * @param page_count number of pages being mapped
 * @param pageSize size of page being added to process
 * @param use use of page
 * @return process space virtual address
 */
uint64_t process_add_page(uint64_t page_number,
                          uint64_t page_count,
                          uint64_t page_Size,
                          process_page_use use);

/**
 * @brief Validates a memory region in a process
 * @param process process being checked agains
 * @param vaddr virtual address checking
 * @param size size of memory region being checked
 * @return 1 for valid 0 for invalid
 */
bool process_validate_address(void* vaddr, size_t size);

int process_fork();

void process_execvp(struct file_t* file, int argc, char** kernel_argv, int envc, char** env);

process_group_t* process_create_group(process_t* parent, process_t* child);
void process_add_to_group(process_t* process, uint64_t pgid);
void process_remove_from_group(process_t* process);

#endif /* PROCESS_H */