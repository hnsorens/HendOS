/* scheduler.h */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <kernel/process.h>

process_t* scheduler_nextProcess();
process_t* scheduler_schedule(process_t* process);
process_t* schedule_end(process_t* process);
process_t* scheduler_currentProcess();

#endif