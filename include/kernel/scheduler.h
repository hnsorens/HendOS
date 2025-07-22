/**
 * @file scheduler.h
 * @brief Kernel Process Scheduler Interface
 *
 * Declares function prototypes for process scheduling and state management in the kernel.
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <kernel/process.h>

process_t* scheduler_nextProcess();
process_t* scheduler_schedule(process_t* process);
process_t* schedule_end(process_t* process);
process_t* scheduler_currentProcess();
void schedule_block(process_t* process);
void schedule_unblock(process_t* process);

#endif