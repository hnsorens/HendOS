#ifndef PID_HASH_TABLE_H
#define PID_HASH_TABLE_H

#include <kint.h>
#include <kstring.h>

#define PID_HASH_BITS 10
#define PID_HASH_SIZE (1 << PID_HASH_BITS)
#define PAGE_SIZE_4KB 4096

typedef struct proc_struct
{
    uint32_t pid;
    // Add process data here
} proc_struct_t;

typedef struct pid_hash_node
{
    uint32_t pid;
    proc_struct_t* proc;
    struct pid_hash_node* next;
} pid_hash_node_t;

typedef struct
{
    pid_hash_node_t* buckets[PID_HASH_SIZE];

    // freelist management for nodes
    pid_hash_node_t* free_nodes;

    // start of virtual memory region reserved for hashmap nodes
    void* nodes_area_start;

    // how many pages allocated for nodes so far
    size_t pages_allocated;
} pid_hash_table_t;

// Initialize the hash table at a fixed virtual start address
// reserve enough space for buckets + nodes allocation starts just after buckets
void pid_hash_init(pid_hash_table_t* table, void* start_virtual_address);

// Insert a process
bool pid_hash_insert(pid_hash_table_t* table, uint32_t pid, proc_struct_t* proc);

// Lookup process by pid
proc_struct_t* pid_hash_lookup(pid_hash_table_t* table, uint32_t pid);

// Delete a process by pid
bool pid_hash_delete(pid_hash_table_t* table, uint32_t pid);

#endif