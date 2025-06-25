#include <kernel/pidHashTable.h>

#include <memory/kglobals.h>
#include <memory/paging.h>
// Simple hash function: mask lower bits
static inline uint32_t pid_hash(uint32_t pid)
{
    return pid & (PID_HASH_SIZE - 1);
}

// Initialize the hash table at a fixed virtual start address
// reserve enough space for buckets + nodes allocation starts just after buckets
void pid_hash_init(pid_hash_table_t* table, void* start_virtual_address)
{
    // Zero out buckets
    for (int i = 0; i < PID_HASH_SIZE; i++)
    {
        table->buckets[i] = 0;
    }

    table->free_nodes = 0;
    table->pages_allocated = 0;
    table->nodes_area_start =
        (void*)((uintptr_t)start_virtual_address + sizeof(pid_hash_node_t) * PID_HASH_SIZE);
}

// Internal: allocate a new page for nodes, map it, and add nodes to freelist
static void alloc_nodes_page(pid_hash_table_t* table)
{
    void* phys_page = pages_allocatePage(PAGE_SIZE_4KB);
    if (!phys_page)
    {
        // handle allocation failure (panic, error return etc)
        return;
    }

    // Virtual address to map the new page to
    void* virt_page =
        (void*)((uintptr_t)table->nodes_area_start + table->pages_allocated * PAGE_SIZE_4KB);

    pageTable_addPage(KERNEL_PAGE_TABLE, virt_page, (uint64_t)phys_page / PAGE_SIZE_4KB, 1,
                      PAGE_SIZE_4KB, 0);

    // Add nodes inside this page to the freelist
    size_t nodes_per_page = PAGE_SIZE_4KB / sizeof(pid_hash_node_t);
    pid_hash_node_t* node_array = (pid_hash_node_t*)virt_page;

    for (size_t i = 0; i < nodes_per_page; i++)
    {
        node_array[i].next = table->free_nodes;
        table->free_nodes = &node_array[i];
    }

    table->pages_allocated++;
}

// Allocate a node from freelist, allocate page if freelist empty
static pid_hash_node_t* alloc_node(pid_hash_table_t* table)
{
    if (!table->free_nodes)
    {
        alloc_nodes_page(table);
        if (!table->free_nodes)
        {
            return 0; // Out of memory
        }
    }

    pid_hash_node_t* node = table->free_nodes;
    table->free_nodes = node->next;
    return node;
}

// Free a node by adding it back to freelist
static void free_node(pid_hash_table_t* table, pid_hash_node_t* node)
{
    node->next = table->free_nodes;
    table->free_nodes = node;
}

// Insert a process
bool pid_hash_insert(pid_hash_table_t* table, uint32_t pid, uint64_t proc)
{
    uint32_t hash = pid_hash(pid);
    pid_hash_node_t* current = table->buckets[hash];
    while (current)
    {
        if (current->pid == pid)
        {
            return false; // Duplicate PID
        }
        current = current->next;
    }

    pid_hash_node_t* new_node = alloc_node(table);
    if (!new_node)
        return false;

    new_node->pid = pid;
    new_node->proc = proc;

    new_node->next = table->buckets[hash];
    table->buckets[hash] = new_node;
    return true;
}

// Lookup process by pid
uint64_t pid_hash_lookup(pid_hash_table_t* table, uint32_t pid)
{
    uint32_t hash = pid_hash(pid);
    pid_hash_node_t* current = table->buckets[hash];
    while (current)
    {
        if (current->pid == pid)
        {
            return current->proc;
        }
        current = current->next;
    }
    return 0;
}

// Delete a process by pid
bool pid_hash_delete(pid_hash_table_t* table, uint32_t pid)
{
    uint32_t hash = pid_hash(pid);
    pid_hash_node_t* current = table->buckets[hash];
    pid_hash_node_t* prev = 0;

    while (current)
    {
        if (current->pid == pid)
        {
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                table->buckets[hash] = current->next;
            }
            free_node(table, current);
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false; // Not found
}