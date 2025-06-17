/**
 * @file process.c
 */

#include <boot/elfLoader.h>
#include <fs/filesystem.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kmath.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <memory/memoryMap.h>
#include <memory/paging.h>
#include <misc/debug.h>

/**
 * @brief Generates a new process id
 * @return Newly generated unique process id
 */
uint64_t process_genPID()
{
    return (*PID)++;
}

/**
 * @brief Translates memory address from user space to kernel space
 * @param addr address to be tranlated
 * @return kernel address
 */
uint64_t process_kernel_address(uint64_t addr)
{
    return (ADDRESS_SECTION_SIZE * (2 + (*CURRENT_PROCESS)->kernel_memory_index) + addr);
}

/**
 * @brief Maps a page to the process
 * @param process process that page gets mapped to
 * @param addr starting address of pages being added
 * @param page_number start page that is being added
 * @param page_count number of pages being mapped
 * @param pageSize size of page being added to process
 * @param use use of page
 * @return process space virtual address
 */
uint64_t process_add_page(uint64_t page_number,
                          uint64_t page_count,
                          uint64_t page_size,
                          process_page_use use)
{
    DECLARE_PROCESS;

    /* userspace address */
    uint64_t user_addr = 0;

    /* Lazy efficient paging for now */
    switch (use)
    {
    case PROCESS_PAGE_HEAP:
        /* Sets address to new heap page address */
        user_addr = ALIGN_UP(process->process_heap_ptr, page_size);
        process->process_heap_ptr = user_addr + page_size;
        break;
    case PROCESS_PAGE_SHARED: /* Starts at 128 gb and goes down */
        /* Sets address to new shared memory address */
        user_addr = process->process_shared_ptr =
            ALIGN_DOWN(process->process_shared_ptr - page_size, page_size);
        break;
    default:
        break;
    }

    /* Adds page at resulting user address */
    pageTable_addPage(process->page_table, process->process_heap_ptr, page_number, page_count,
                      page_size, 0);

    /* Adds page to kernel as well */
    uint64_t kernel_page_offset = process_kernel_address(0) / page_size;
    pageTable_addPage(process->page_table, process->process_heap_ptr,
                      page_number + kernel_page_offset, page_count, page_size, 0);

    return user_addr;
}

/**
 * @brief Validates a memory region in a process
 * @param vaddr virtual address checking
 * @param size size of memory region being checked
 * @return 1 for valid 0 for invalid
 */
bool process_validate_address(void* vaddr, size_t size)
{
    DECLARE_PROCESS;

    size_t start = (size_t)vaddr;
    size_t end = start + size;

    // TODO: implement this somehow

    return 1;
}

void process_remove_from_group(process_t* process)
{
    if (process->pgid == 0)
        return;
    process_group_t* group = pid_hash_lookup(PGID_MAP, process->pgid);
    for (int i = 0; i < group->process_count; i++)
    {
        if (group->processes[i]->pid == process->pid)
        {
            group->processes[i] = group->processes[--group->process_count];
            process->pgid = 0;
        }
    }
}

void process_add_to_group(process_t* process, uint64_t pgid)
{
    process_group_t* group = pid_hash_lookup(PGID_MAP, pgid);

    if (!group)
    {
        group = process_create_group(pgid);
    }

    if (group->process_count == group->process_capacity)
    {
        group->process_capacity *= 2;
        group->processes = krealloc(group->processes, group->process_capacity * sizeof(process_t*));
    }
    group->processes[group->process_count++] = process;
    process->pgid = group->pgid;
}

process_group_t* process_create_group(uint64_t pgid)
{
    process_group_t* group = kmalloc(sizeof(process_group_t));

    group->pgid = pgid;
    group->process_capacity = 1;
    group->processes = kmalloc(sizeof(process_t*) * group->process_capacity);

    pid_hash_insert(PGID_MAP, pgid, group);

    return group;
}

int process_fork()
{
    process_t* forked_process = (*CURRENT_PROCESS);
    process_t* process = kaligned_alloc(sizeof(process_t), 16);
    kmemcpy(process, forked_process, sizeof(process_t));
    process->page_table = pageTable_fork(forked_process->page_table);
    process->process_stack_signature.rax = 0;
    forked_process->process_stack_signature.rax = 1;
    process->pid = process_genPID();
    scheduler_schedule(process);
}

void process_execvp(file_t* file, int argc, char** kernel_argv, int envc, char** env)
{
    page_table_t* page_table = pageTable_createPageTable();

    process_t* process = *CURRENT_PROCESS;
    int kernel_memory_index = elfLoader_load(page_table, file, process);

    void* stackPage = pages_allocatePage(PAGE_SIZE_2MB);

    uint64_t pid = process_genPID();

    process->kernel_memory_index = kernel_memory_index;
    process->page_table = page_table;
    process->pid = pid;
    process->stackPointer = 0x7FFF00;           /* 5mb + 1kb */
    process->process_heap_ptr = 0x40000000;     /* 1 gb */
    process->process_shared_ptr = 0x2000000000; /* 128gb */

    process->process_stack_signature.r15 = 0;
    process->process_stack_signature.r14 = 0;
    process->process_stack_signature.r13 = 0;
    process->process_stack_signature.r12 = 0;
    process->process_stack_signature.r11 = 0;
    process->process_stack_signature.r10 = 0;
    process->process_stack_signature.r9 = 0;
    process->process_stack_signature.r8 = 0;
    process->process_stack_signature.rbp = 0;
    process->process_stack_signature.rdi = 0;
    process->process_stack_signature.rsi = 0;
    process->process_stack_signature.rdx = 0;
    process->process_stack_signature.rcx = 0;
    process->process_stack_signature.rbx = 0;
    process->process_stack_signature.rax = 0;
    process->process_stack_signature.cs = 0x1B; /* kernel - 0x08, user - 0x1B*/
    process->process_stack_signature.rflags = (1 << 9) | (1 << 1);
    process->process_stack_signature.rsp = 0x7FFF00; /* 5mb + 1kb */
    process->process_stack_signature.ss = 0x23;      /* kernel - 0x10, user - 0x23 */
    process->flags = 0;
    process->cwd = file->dir;
    process->heap_end = 0x40000000; /* 1gb */

    pageTable_addPage(page_table, 0x600000, (uint64_t)stackPage / PAGE_SIZE_2MB, 1, PAGE_SIZE_2MB,
                      4);
    pageTable_addPage(KERNEL_PAGE_TABLE,
                      (ADDRESS_SECTION_SIZE * (2 + kernel_memory_index)) + 0x600000 /* 5mb */,
                      (uint64_t)stackPage / PAGE_SIZE_2MB, 1, PAGE_SIZE_2MB, 0);

    /* Configure arguments */
    void* args_page = pages_allocatePage(PAGE_SIZE_2MB);
    pageTable_addPage(page_table, 0x200000, (uint64_t)args_page / PAGE_SIZE_2MB, 1, PAGE_SIZE_2MB,
                      4);
    pageTable_addPage(KERNEL_PAGE_TABLE,
                      (ADDRESS_SECTION_SIZE * (2 + kernel_memory_index)) + 0x200000 /* 2mb */,
                      (uint64_t)args_page / PAGE_SIZE_2MB, 1, PAGE_SIZE_2MB, 0);
    return 0;
}