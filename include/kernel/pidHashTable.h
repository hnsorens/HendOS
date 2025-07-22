/**
 * @file pidHashTable.h
 * @brief PID Hash Table Interface
 *
 * Provides a hash table implementation for mapping process IDs (PIDs) to process structures.
 */

#ifndef PID_HASH_TABLE_H
#define PID_HASH_TABLE_H

#include <kint.h>
#include <kstring.h>

/* Configuration constants */
#define PID_HASH_BITS 10                   ///< Number of bits for hash (determines table size)
#define PID_HASH_SIZE (1 << PID_HASH_BITS) ///< Total number of buckets (1024)
#define PAGE_SIZE_4KB 4096                 ///< Page size for node allocation

/**
 * @struct pid_hash_node_t
 * @brief Hash table node structure
 */
typedef struct pid_hash_node
{
    uint32_t pid;               ///< Process ID key
    uint64_t proc;              ///< Pointer to process structure
    struct pid_hash_node* next; ///< Next node in chain
} pid_hash_node_t;

/**
 * @struct pid_hash_table_t
 * @brief PID hash table main structure
 */
typedef struct
{
    pid_hash_node_t* buckets[PID_HASH_SIZE]; ///< Array of bucket pointers

    /* Memory management fields */
    pid_hash_node_t* free_nodes; ///< Freelist of available nodes
    void* nodes_area_start;      ///< Start of virtual memory for nodes
    size_t pages_allocated;      ///< Number of pages allocated for nodes
} pid_hash_table_t;

/* ==================== Public Interface ==================== */

/**
 * @brief Initialize PID hash table
 * @param table Pointer to hash table structure
 * @param start_virtual_address Virtual address to reserve for hash table
 *
 * Initializes the hash table at a fixed virtual address, setting up:
 * - Empty buckets
 * - Empty freelist
 * - Memory management tracking
 */
void pid_hash_init(pid_hash_table_t* table, void* start_virtual_address);

/**
 * @brief Insert process into hash table
 * @param table Hash table pointer
 * @param pid Process ID to insert
 * @param proc Process structure pointer
 * @return true if inserted, false if duplicate or allocation failed
 */
bool pid_hash_insert(pid_hash_table_t* table, uint32_t pid, uint64_t proc);

/**
 * @brief Lookup process by PID
 * @param table Hash table pointer
 * @param pid Process ID to find
 * @return Process structure pointer or 0 if not found
 */
uint64_t pid_hash_lookup(pid_hash_table_t* table, uint32_t pid);

/**
 * @brief Delete process from hash table
 * @param table Hash table pointer
 * @param pid Process ID to remove
 * @return true if found and deleted, false if not found
 */
bool pid_hash_delete(pid_hash_table_t* table, uint32_t pid);

#endif /* PID_HASH_TABLE_H */