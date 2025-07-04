/**
 * @file pidHashTable.c
 * @brief PID Hash Table Implementation
 *
 * Implements a memory-efficient hash table for process ID tracking with:
 * - Custom page-based node allocation
 * - Freelist management for recycled nodes
 * - Basic hash table operations
 */

#include <kernel/pidHashTable.h>
#include <memory/kglobals.h>
#include <memory/pmm.h>

/**
 * @brief Compute hash value for PID
 * @param pid Process ID to hash
 * @return Bucket index (0 to PID_HASH_SIZE-1)
 *
 * Uses simple bitmasking for fast hashing since PIDs are
 * expected to be randomly distributed.
 */
static inline uint32_t pid_hash(uint32_t pid)
{
    return pid & (PID_HASH_SIZE - 1);
}

/**
 * @brief Initialize hash table at fixed virtual address
 * @param table Hash table to initialize
 * @param start_virtual_address Virtual memory region start
 *
 * Sets up:
 * - Empty bucket array
 * - Memory management tracking
 * - Freelist initialization
 */
void pid_hash_init(pid_hash_table_t* table, void* start_virtual_address)
{
    /* Initialize all buckets to empty */
    for (int i = 0; i < PID_HASH_SIZE; i++)
    {
        table->buckets[i] = 0;
    }

    /* Initialize memory management */
    table->free_nodes = 0;
    table->pmm_allocated = 0;
    /* Node area starts after the bucket pointers */
    table->nodes_area_start = (void*)((uintptr_t)start_virtual_address + sizeof(pid_hash_node_t) * PID_HASH_SIZE);
}

/**
 * @brief Allocate new page for hash nodes
 * @param table Hash table to allocate for
 *
 * Allocates a physical page, maps it to the next virtual address
 * in the node area, and adds all nodes in the page to the freelist.
 */
static void alloc_nodes_page(pid_hash_table_t* table)
{
    /* Allocate physical page */
    void* phys_page = pmm_allocate(PAGE_SIZE_4KB);
    if (!phys_page)
    {
        /* Handle allocation failure */
        return;
    }

    /* Calculate virtual address for new page */
    void* virt_page = (void*)((uintptr_t)table->nodes_area_start + table->pmm_allocated * PAGE_SIZE_4KB);

    /* Map the physical page to virtual address */
    vmm_add_page(KERNEL_PAGE_TABLE, virt_page, (uint64_t)phys_page / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB, 0);

    /* Add all nodes in page to freelist */
    size_t nodes_per_page = PAGE_SIZE_4KB / sizeof(pid_hash_node_t);
    pid_hash_node_t* node_array = (pid_hash_node_t*)virt_page;

    for (size_t i = 0; i < nodes_per_page; i++)
    {
        node_array[i].next = table->free_nodes;
        table->free_nodes = &node_array[i];
    }

    table->pmm_allocated++;
}

/**
 * @brief Allocate a hash node
 * @param table Hash table to allocate from
 * @return New node or NULL if out of memory
 *
 * Gets node from freelist or allocates new page if freelist is empty.
 */
static pid_hash_node_t* alloc_node(pid_hash_table_t* table)
{
    /* Allocate new page if freelist is empty */
    if (!table->free_nodes)
    {
        alloc_nodes_page(table);
        if (!table->free_nodes)
        {
            return 0; /* Out of memory */
        }
    }

    /* Get node from head of freelist */
    pid_hash_node_t* node = table->free_nodes;
    table->free_nodes = node->next;
    return node;
}

/**
 * @brief Free a hash node
 * @param table Hash table to return node to
 * @param node Node to free
 *
 * Returns node to freelist for reuse.
 */
static void free_node(pid_hash_table_t* table, pid_hash_node_t* node)
{
    node->next = table->free_nodes;
    table->free_nodes = node;
}

/**
 * @brief Insert process into hash table
 * @param table Hash table pointer
 * @param pid Process ID to insert
 * @param proc Process structure pointer
 * @return true if inserted, false if duplicate or allocation failed
 */
bool pid_hash_insert(pid_hash_table_t* table, uint32_t pid, uint64_t proc)
{
    uint64_t current_cr3;
    __asm__ volatile("mov %%cr3, %0\n\t" : "=r"(current_cr3) : :);
    __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(*KERNEL_PAGE_TABLE) :);
    uint32_t hash = pid_hash(pid);

    /* Check for duplicate PID */
    pid_hash_node_t* current = table->buckets[hash];
    while (current)
    {
        if (current->pid == pid)
        {
            return false; /* Duplicate PID */
        }
        current = current->next;
    }

    /* Allocate new node */
    pid_hash_node_t* new_node = alloc_node(table);
    if (!new_node)
    {
        __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(current_cr3) :);
        return false;
    }

    /* Initialize and insert new node */
    new_node->pid = pid;
    new_node->proc = proc;
    new_node->next = table->buckets[hash];
    table->buckets[hash] = new_node;
    __asm__ volatile("mov %0, %%cr3\n\t" ::"r"(current_cr3) :);
    return true;
}

/**
 * @brief Lookup process by PID
 * @param table Hash table pointer
 * @param pid Process ID to find
 * @return Process structure pointer or 0 if not found
 */
uint64_t pid_hash_lookup(pid_hash_table_t* table, uint32_t pid)
{
    uint32_t hash = pid_hash(pid);
    pid_hash_node_t* current = table->buckets[hash];

    /* Search bucket chain */
    while (current)
    {
        if (current->pid == pid)
        {
            return current->proc;
        }
        current = current->next;
    }
    return 0; /* Not found */
}

/**
 * @brief Delete process from hash table
 * @param table Hash table pointer
 * @param pid Process ID to remove
 * @return true if found and deleted, false if not found
 */
bool pid_hash_delete(pid_hash_table_t* table, uint32_t pid)
{
    uint32_t hash = pid_hash(pid);
    pid_hash_node_t* current = table->buckets[hash];
    pid_hash_node_t* prev = 0;

    /* Search bucket chain */
    while (current)
    {
        if (current->pid == pid)
        {
            /* Remove from chain */
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                table->buckets[hash] = current->next;
            }

            /* Return node to freelist */
            free_node(table, current);
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false; /* Not found */
}