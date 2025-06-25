/* scheduler.c */
#include <kernel/scheduler.h>
#include <memory/kglobals.h>
#include <misc/debug.h>

process_t* scheduler_currentProcess()
{
    return *PROCESSES;
}

process_t* scheduler_nextProcess()
{
    if (!(*PROCESSES))
        return 0;

    process_t* current = *PROCESSES;
    do
    {
        current = current->next;
    } while (current->flags & PROCESS_BLOCKING);

    *PROCESSES = current;
    return *PROCESSES;
}

process_t* scheduler_schedule(process_t* process)
{
    if (!process)
        return;

    pid_hash_insert(PID_MAP, process->pid, process);

    (*PROCESS_COUNT)++;
    if (!(*PROCESSES))
    {
        process->next = process;
        process->last = process;
        *PROCESSES = process;
        return 0;
    }
    else
    {
        (*PROCESSES)->next->last = process;
        process->next = (*PROCESSES)->next;
        (*PROCESSES)->next = process;
        process->last = (*PROCESSES);
        *PROCESSES = process;
        return (*PROCESSES);
    }
}

process_t* schedule_end(process_t* process)
{
    if (!process)
        return;

    (*PROCESS_COUNT)--;
    if (process == process->next == process->last)
    {
        kfree(process);
        *PROCESSES = 0;
        return 0;
    }
    else
    {
        if (process == *PROCESSES)
            scheduler_nextProcess();
        process->next->last = process->last;
        process->last->next = process->next;
        kfree(process);
        return *PROCESSES;
    }
}

void schedule_block(process_t* process)
{
    process->flags |= PROCESS_BLOCKING;
}

void schedule_unblock(process_t* process)
{
    process->flags &= ~PROCESS_BLOCKING;
}