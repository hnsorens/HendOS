/* scheduler.c */
#include <kernel/scheduler.h>
#include <memory/kglobals.h>

process_t* scheduler_currentProcess()
{
    return *PROCESSES;
}

process_t* scheduler_nextProcess()
{
    if (!(*PROCESSES))
        return 0;
    *PROCESSES = (*PROCESSES)->next;
    return *PROCESSES;
}

process_t* scheduler_schedule(process_t* process)
{
    if (!process)
        return;

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
        process_t* returnProcess = *PROCESSES;
        if (process == *PROCESSES)
            returnProcess = (*PROCESSES)->next;
        process->next->last = process->last;
        process->last->next = process->next;
        kfree(process);
        return returnProcess;
    }
}
