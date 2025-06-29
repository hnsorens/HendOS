/**
 * @file process.c
 */

#include <boot/elfLoader.h>

#include <fs/vfs.h>
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
 * @brief Maps a page to the process
 * @param process process that page gets mapped to
 * @param addr starting address of pages being added
 * @param page_number start page that is being added
 * @param page_count number of pages being mapped
 * @param pageSize size of page being added to process
 * @param use use of page
 * @return process space virtual address
 */
uint64_t process_add_page(uint64_t page_number, uint64_t page_count, uint64_t page_size, process_page_use use)
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
        user_addr = process->process_shared_ptr = ALIGN_DOWN(process->process_shared_ptr - page_size, page_size);
        break;
    default:
        break;
    }

    /* Adds page at resulting user address */
    pageTable_addPage(&process->page_table, process->process_heap_ptr, page_number, page_count, page_size, 4);

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

void process_remove_from_session(process_t* process)
{
    if (process->sid == 0)
        return;
    process_session_t* session = pid_hash_lookup(SID_MAP, process->sid);
    for (int i = 0; i < session->process_count; i++)
    {
        if (session->processes[i]->sid == process->sid)
        {
            session->processes[i] = session->processes[--session->process_count];
            process->sid = 0;
        }
    }
}

process_group_t* process_create_group(uint64_t pgid)
{
    process_group_t* group = pool_allocate(*PROCESS_GROUP_POOL);

    group->pgid = pgid;
    group->process_capacity = 1;
    group->process_count = 0;
    group->processes = kmalloc(sizeof(process_t*) * group->process_capacity);
    pid_hash_insert(PGID_MAP, pgid, group);

    return group;
}

process_session_t* process_create_session(uint64_t sid)
{
    process_session_t* session = pool_allocate(*SESSION_POOL);

    session->sid = sid;
    session->process_capacity = 1;
    session->process_count = 0;
    session->processes = kmalloc(sizeof(process_t*) * session->process_capacity);
    pid_hash_insert(SID_MAP, sid, session);

    return session;
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

void process_add_to_session(process_t* process, uint64_t sid)
{
    process_session_t* session = pid_hash_lookup(SID_MAP, sid);

    if (!session)
    {
        session = process_create_session(sid);
    }

    if (session->process_count == session->process_capacity)
    {
        session->process_capacity *= 2;
        session->processes = krealloc(session->processes, session->process_capacity * sizeof(process_t*));
    }
    session->processes[session->process_count++] = process;
    process->pgid = session->sid;
}

int process_fork()
{
    process_t* forked_process = (*CURRENT_PROCESS);
    process_t* process = pool_allocate(*PROCESS_POOL);
    kmemcpy(process, forked_process, sizeof(process_t));
    process->file_descriptor_table = kmalloc(sizeof(file_descriptor_t) * process->file_descriptor_capacity);
    kmemcpy(process->file_descriptor_table, forked_process->file_descriptor_table, sizeof(file_descriptor_t) * process->file_descriptor_capacity);
    process->page_table = pageTable_fork(&forked_process->page_table);
    process->pid = process_genPID();
    process->process_stack_signature.rax = 0;
    forked_process->process_stack_signature.rax = process->pid;
    process->waiting_parent_pid = 0;
    process->flags = 0;
    process->signal = SIGNONE;
    pageTable_addKernel(&process->page_table);
    (*CURRENT_PROCESS) = scheduler_schedule(process);
    /* Prepare for context switch:
     * R12 = new process's page table root (CR3)
     * R11 = new process's stack pointer */
    INTERRUPT_INFO->cr3 = (*CURRENT_PROCESS)->page_table;
    INTERRUPT_INFO->rsp = &(*CURRENT_PROCESS)->process_stack_signature;
    TSS->ist1 = (uint64_t)(*CURRENT_PROCESS) + sizeof(process_stack_layout_t);
}

void process_execvp(open_file_t* file, int argc, char** kernel_argv, int envc, char** env)
{
    uint64_t current_cr3;
    __asm__ volatile("mov %%cr3, %0\n\t" : "=r"(current_cr3) : :);
    __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(*KERNEL_PAGE_TABLE) :);

    page_table_t page_table = 0;

    process_t* process = *CURRENT_PROCESS;
    elfLoader_load(&page_table, file, process);
    void* stackPage = pages_allocatePage(PAGE_SIZE_2MB);

    process->page_table = page_table;
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
    process->heap_end = 0x40000000;                  /* 1gb */
    process->signal = SIGNONE;

    pageTable_addPage(&process->page_table, 0x600000, (uint64_t)stackPage / PAGE_SIZE_2MB, 1, PAGE_SIZE_2MB, 4);

    /* Configure arguments */
    void* args_page = pages_allocatePage(PAGE_SIZE_2MB);
    pageTable_addPage(&process->page_table, 0x200000, (uint64_t)args_page / PAGE_SIZE_2MB, 1, PAGE_SIZE_2MB, 4);

    *((uint64_t*)(0x7FFF00)) = argc;
    int current_offset = 0x200000;
    for (int i = 0; i < argc; i++)
    {
        kmemcpy(current_offset, kernel_argv[i], kernel_strlen(kernel_argv[i]) + 1);
        *((uint64_t*)(0x7FFF08 + i * 8)) = current_offset;
        current_offset += kernel_strlen(kernel_argv[i]) + 1;
    }

    __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(current_cr3) :);
    return 0;
}

uint64_t process_cleanup(process_t* process)
{
    uint64_t status = process->status;
    pool_free(process);
    return status;
}

void process_signal(process_t* process, sig_t signal)
{
    if (signal == SIGKILL)
    {
        /* Exit process Immediately */
    }

    (*CURRENT_PROCESS)->signal = signal;
}

void process_group_signal(process_group_t* group, sig_t signal)
{
    for (int i = 0; i < group->process_count; i++)
    {
        process_signal(group->processes[i], signal);
    }
}

void process_signal_all(sig_t signal)
{
    for (int i = 0; i < PID_HASH_SIZE; i++)
    {
        pid_hash_node_t* current = PID_MAP->buckets[i];

        while (current)
        {
            process_signal(current->proc, signal);
        }
    }
}