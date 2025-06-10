/**
 * @file device.c
 */

#include <kernel/device.h>
#include <kernel/process.h>
#include <kmath.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <memory/paging.h>

/* ============================= Private API ===================================== */

/**
 * @brief Adds device to Device Manager and sets id
 * @param device Device to add
 * @param id id pointer
 * @return error code
 */
static uint64_t register_device(device_t* device, uint64_t* id)
{
    /* Grows device array if needed */
    if (DEVICE_MANAGER->device_count == DEVICE_MANAGER->device_capacity)
    {
        DEVICE_MANAGER->device_capacity *= 2;
        DEVICE_MANAGER->devices =
            krealloc(DEVICE_MANAGER->devices, sizeof(device_t*) * DEVICE_MANAGER->device_capacity);
        if (!DEVICE_MANAGER)
        {
            return 1; /* Kernel Heap Overflow */
        }
    }

    /* Sets new device */
    *id = DEVICE_MANAGER->device_count;
    DEVICE_MANAGER->devices[DEVICE_MANAGER->device_count++] = device;

    return 0;
}

/**
 * @brief Removes a device from the device manager
 * @param id Device to remove
 * @return error code
 */
static uint64_t unregister_device(uint64_t device_id)
{
    /* Replaces device with last device*/
    DEVICE_MANAGER->devices[device_id] = 0;

    // TODO: make a free stack for devices

    return 0;
}

/**
 * @brief Adds a callback event to a callback queue
 */
static void schedule_callback(uint64_t dev_id, uint64_t fn_id, callback_args_t args)
{
    device_t* device = DEVICE_MANAGER->devices[dev_id];
    callback_event_queue_entry_t entry;
    entry.fn_id = fn_id;
    entry.args = args;
    device->callback_event_queue->data[device->callback_event_queue->head++] = entry;
    if (device->callback_event_queue->head >= DEV_CALLBACK_QUEUE_ENTRY_COUNT)
    {
        device->callback_event_queue->head = 0;
    }
    if (device->callback_event_queue->head == device->callback_event_queue->tail)
    {
        device->callback_event_queue->tail++;
    }
}

/* ============================= Public API ====================================== */

/**
 * @brief Initialized the Device Manager
 */
void dev_init(void)
{
    DEVICE_MANAGER->device_count = 0;
    DEVICE_MANAGER->device_capacity = 64;
    DEVICE_MANAGER->devices = kmalloc(sizeof(device_t*) * DEVICE_MANAGER->device_capacity);
}

/**
 * @brief Registers a kernel callback function.
 * @param dev_id Device identifier.
 * @param fn_id Kernel function id
 * @param write Write function
 */
uint64_t dev_register_kernel_callback(uint64_t dev_id, uint64_t fn_id, kernel_fn function)
{
    /* Registers Callback */
    device_t* device = DEVICE_MANAGER->devices[dev_id];
    dev_callback_signature signature;
    signature.kernel_function_ptr = (uint64_t)function;
    device->vtable[fn_id] = signature;
    return 0;
}

/**
 * @brief Runs a kernel device function
 * @param dev_id Device identifier.
 * @param fn_id Kernel device id
 * @param arg1 First argument
 * @param arg2 Second argument
 */
uint64_t dev_kernel_fn(uint64_t dev_id, uint64_t fn_id, uint64_t arg1, uint64_t arg2)
{
    device_t* device = DEVICE_MANAGER->devices[dev_id];
    return device->vtable[fn_id].kernel_function_ptr(arg1, arg2);
}

/* ============================= Device Syscalls ================================= */

/**
 * @brief Retrieves the number of registered devices.
 * @param count [out] Pointer where device count will be stored.
 */
uint64_t dev_count()
{
    return DEVICE_MANAGER->device_count;
}

/**
 * @brief Lists all current device IDs.
 * @param buffer [out] Pointer to a buffer where device IDs will be copied.
 * @param count Number of device IDs expected in the buffer.
 * @return number of devices recieved
 */
uint64_t dev_list(size_t buffer, size_t count)
{
    // TODO: implement a user space safety function that takes in a pointer and a size and
    // determines if its safe memory or not

    /* Makes sure count is in the range of number of devices */
    uint64_t devices_copied = min(count, DEVICE_MANAGER->device_count);
    kmemcpy(process_kernel_address(buffer), DEVICE_MANAGER->devices, devices_copied);
    return devices_copied;
}

/**
 * @brief Maps read-only metadata for a specific device into user-space.
 * @param dev_id Device identifier.
 * @param ptr [out] Pointer where mapped metadata address will be stored.
 */
uint64_t dev_info(uint64_t dev_id, size_t* ptr)
{
    /* Gets the device and maps memory to current process */
    device_t* device = DEVICE_MANAGER->devices[dev_id];
    // TODO: add process page adder that adds pages to a processes

    return 0;
}

/**
 * @brief Opens a device and requests access.
 * @param dev_id Device identifier.
 */
uint64_t dev_open(uint64_t dev_id)
{
    schedule_callback(dev_id, DEV_OPEN, DEV_NO_ARGS); // TODO: Add args if need be
    return 0;
}

/**
 * @brief Closes access to a device and unmaps any shared memory.
 * @param dev_id Device identifier.
 */
uint64_t dev_close(uint64_t dev_id)
{
    schedule_callback(dev_id, DEV_CLOSE, DEV_NO_ARGS); // TODO: Add args if need be

    // TODO: Remove Access to process

    return 0;
}

/**
 * @brief Creates a new device with the specified path and flags.
 * @param path Path to the new device.
 * @param flags Access and permission flags.
 */
uint64_t dev_create(const char* path, uint64_t flags)
{
    /* Allocates new device and adds it to the Device Manager*/
    device_t* device = kmalloc(sizeof(device_t));
    kmemset(device, 0, sizeof(device_t));
    uint64_t device_id;
    register_device(device, &device_id);

    /* 4kb for device callback queue */
    device->callback_event_queue_size = sizeof(callback_event_queue_t);
    device->callback_event_queue = pages_allocatePage(device->callback_event_queue_size);
    device->callback_event_queue->head = 0;
    device->callback_event_queue->tail = 0;

    /* 4kb for device info */
    device->device_info_size = PAGE_SIZE_4KB;
    device->device_info = pages_allocatePage(device->device_info_size);

    return device_id;
}

/**
 * @brief Deletes a device permanently.
 * @param dev_id Device identifier.
 */
uint64_t dev_destroy(uint64_t dev_id)
{
    /* Removes device and frees it */
    device_t* device = DEVICE_MANAGER->devices[dev_id];
    unregister_device(dev_id);
    kfree(device);

    // TODO: run dev_close on all current processes that have the device

    return 0;
}

/**
 * @brief Registers a callback function with a given signature.
 * @param dev_id Device identifier.
 * @param fn_id Function ID (used in dev_call).
 * @param signature Pointer to the callback signature.
 */
uint64_t dev_register_callback(uint64_t dev_id, uint64_t fn_id, dev_callback_signature* signature)
{
    signature = process_kernel_address(signature);

    /* validate signature */
    if (*((uint64_t*)signature) != DEV_CALLBACK_SIGNATURE_MAGIC)
        return 1;

    /* Check to make sure each pointer has a size handle */
    for (int i = 0; i < MAX_DEV_CALLBACK_ARGS; i++)
    {
        /* Checks if arg type is pointer */
        dev_arg_type arg_type = signature->args[i].arg_type;
        if (arg_type == DEV_ARG_PTR_IN || arg_type == DEV_ARG_PTR_INOUT ||
            arg_type == DEV_ARG_PTR_OUT)
        {
            /* Checks size handle */
            size_t size = signature->args[i].dev_size;
            if (signature->args[i].size_type == DEV_SIZE_DYNAMIC)
            {
                /* Return if size handle is invalid or the wrong type */
                if (size < 0 || size >= MAX_DEV_CALLBACK_ARGS ||
                    signature->args[size].arg_type != DEV_ARG_INT)
                {
                    return 1;
                }
            }
        }
    }

    /* Registers Callback */
    device_t* device = DEVICE_MANAGER->devices[dev_id];
    kmemcpy(&device->vtable[fn_id], process_kernel_address(signature),
            sizeof(dev_callback_signature));

    return 0;
}

/**
 * @brief Unregisters a previously registered callback.
 * @param dev_id Device identifier.
 * @param fn_id Function ID to unregister.
 */
uint64_t dev_unregister_callback(uint64_t dev_id, uint64_t fn_id)
{
    /* Gets the device */
    device_t* device = DEVICE_MANAGER->devices[dev_id];

    /* Sets whole signature to 0 (invalidates magic) */
    kmemset(&device->vtable[fn_id], 0, sizeof(dev_callback_signature));

    return 0;
}

/**
 * @brief Maps a device's callback queue into user-space (only for trusted processes).
 * @param dev_id Device identifier.
 * @return queue address
 */
uint64_t dev_map_queue(uint64_t dev_id)
{
    /* Maps page to process */
    device_t* device = DEVICE_MANAGER->devices[dev_id];
    return process_add_page((uint64_t)device->callback_event_queue / PAGE_SIZE_4KB,
                            device->callback_event_queue_size / PAGE_SIZE_4KB, PAGE_SIZE_4KB,
                            PROCESS_PAGE_SHARED);
}

/**
 * @brief Unmaps a previously mapped callback queue.
 * @param dev_id Device identifier.
 */
uint64_t dev_unmap_queue(uint64_t dev_id)
{
    /* Unmaps page to process */
    device_t* device = DEVICE_MANAGER->devices[dev_id];
    // TODO: create pageTable_removePage()
    return 0;
}

/**
 * @brief Blocks the current process until a callback event is received.
 */
uint64_t dev_poll()
{
    return 0;
}

/**
 * @brief Waits for an event with a timeout (in milliseconds).
 * @param dev_id Device identifier.
 * @param timeout Maximum wait time.
 */
uint64_t dev_wait_event(uint64_t dev_id, uint64_t timeout)
{
    return 0;
}

/**
 * @brief Makes a device API call with up to 6 arguments.
 * @param dev_id Device identifier.
 * @param fn_id Function ID.
 * @param arg1-arg6 Arguments for the function.
 */
uint64_t dev_call(uint64_t dev_id, uint64_t fn_id, callback_args_t* args)
{
    callback_args_t args_copy = *(callback_args_t*)process_kernel_address(args);
    schedule_callback(dev_id, fn_id, args_copy);
    return 0;
}

/**
 * @brief Shares user-allocated memory with a device.
 * @param dev_id Device identifier.
 * @param ptr Pointer to memory to share (must be 4KB-aligned).
 * @param size Size of memory (must be a multiple of 4KB).
 * @return resulting vaddr in device user space
 */
uint64_t dev_share(uint64_t dev_id, size_t ptr, size_t size)
{
    uint64_t page_count = size / PAGE_SIZE_4KB;
    size_t kernel_ptr = process_kernel_address(ptr);
    return process_add_page(kernel_ptr / PAGE_SIZE_4KB, page_count, PAGE_SIZE_4KB,
                            PROCESS_PAGE_SHARED);
}

/**
 * @brief Grants access to a device for all processes in a group.
 * @param dev_id Device identifier.
 * @param gid Group identifier.
 */
uint64_t dev_grant_access(uint64_t dev_id, uint64_t gid)
{
    return 0;
}

/**
 * @brief Grants trust to a group (enables queue mapping).
 * @param dev_id Device identifier.
 * @param gid Group identifier.
 */
uint64_t dev_grant_trust(uint64_t dev_id, uint64_t gid)
{
    return 0;
}

/**
 * @brief Revokes access to a device from a group.
 */
uint64_t dev_revoke_access(uint64_t dev_id, uint64_t gid)
{
    return 0;
}

/**
 * @brief Revokes trust from a group.
 */
uint64_t dev_revoke_trust(uint64_t dev_id, uint64_t gid)
{
    return 0;
}

/**
 * @brief Changes the device owner to another UID.
 */
uint64_t dev_set_owner(uint64_t dev_id, uint64_t uid)
{
    return 0;
}

/**
 * @brief Renames a device.
 */
uint64_t dev_rename(const char* old_path, const char* new_path)
{
    return 0;
}
