/**
 * @file device.h
 * @brief High-level Device API for user-mode and kernel interaction.
 *
 * This API provides a modern, secure, and high-performance abstraction layer
 * for managing devices from user-space, including access control, memory sharing,
 * callback registration, and device function invocation.
 */

#ifndef DEVICE_H
#define DEVICE_H

#include <kint.h>

#define DEV_NO_ARGS                                                                                \
    (callback_args_t)                                                                              \
    {                                                                                              \
        0, 0, 0, 0, 0, 0                                                                           \
    }
#define MAX_DEV_CALLBACK_ARGS 6
#define MAX_DEV_CALLBACKS 128
#define DEV_WRITE 124
#define DEV_READ 125
#define DEV_OPEN 126
#define DEV_CLOSE 127
#define DEV_CALLBACK_SIGNATURE_MAGIC 0x4B424C4C43455644
#define DEV_CALLBACK_QUEUE_ELEMENT_SIZE 56
#define DEV_CALLBACK_QUEUE_ENTRY_COUNT 73

typedef uint64_t (*kernel_fn)(uint64_t arg1, uint64_t arg2);

/**
 * @brief Initialized the Device Manager
 */
void dev_init(void);

/* ========================== Device Syscalls ================================*/

/**
 * @enum device callback argument
 */
typedef enum dev_arg_type
{
    DEV_ARG_INT,       ///< Simple 64-bit integer argument
    DEV_ARG_PTR_IN,    ///< Pointer input (read-only by the device)
    DEV_ARG_PTR_OUT,   ///< Pointer output (write-only by the device)
    DEV_ARG_PTR_INOUT, ///< Pointer input/output (read-write by the device)
    DEV_ARG_NONE,      ///< No argument
} dev_arg_type;

/**
 * @enum device size handle type
 */
typedef enum dev_size_type
{
    DEV_SIZE_STATIC, ///< Argument size is fixed
    DEV_SIZE_DYNAMIC ///< Argument size is specified at runtime
} dev_size_type;

/**
 * @brief Argument type and metadata for device callback registration.
 */
typedef struct dev_callback_args
{
    dev_arg_type arg_type;
    dev_size_type size_type;

    size_t dev_size; ///< Size of argument, or dynamic if dev_size_type is DEV_SIZE_DYNAMIC
} dev_callback_args;

/**
 * @struct Signature describing a device callback function.
 */
typedef struct dev_callback_signature
{
    uint8_t magic[7]; ///< Signature verification (magic bytes)
    uint8_t id;
    kernel_fn kernel_function_ptr;                 ///< Number of arguments
    dev_callback_args args[MAX_DEV_CALLBACK_ARGS]; ///< Argument descriptions
} dev_callback_signature;

/**
 * @struct device callback arguments
 */
typedef struct callback_args_t
{
    uint64_t arg_0;
    uint64_t arg_1;
    uint64_t arg_2;
    uint64_t arg_3;
    uint64_t arg_4;
    uint64_t arg_5;
} __attribute__((packed)) callback_args_t;

/**
 * @struct callback event queue entry
 */
typedef struct callback_event_queue_entry_t
{
    uint64_t fn_id;
    callback_args_t args;
} __attribute__((packed)) callback_event_queue_entry_t;

/**
 * @struct callback event queue
 */
typedef struct callback_event_queue_t
{
    uint32_t head;
    uint32_t tail;
    callback_event_queue_entry_t data[DEV_CALLBACK_QUEUE_ENTRY_COUNT];
} __attribute__((packed)) callback_event_queue_t;

/**
 * @struct Device description
 *
 * Reserved Calbacks:
 * 124 - write()
 * 125 - read()
 * 126 - open()
 * 127 - close()
 */
typedef struct device_t
{
    void* device_info;
    size_t device_info_size;

    callback_event_queue_t* callback_event_queue;
    size_t callback_event_queue_size;

    dev_callback_signature vtable[MAX_DEV_CALLBACKS];
} device_t;

typedef struct dev_manager_t
{
    device_t** devices;
    uint64_t device_count;
    uint64_t device_capacity;
} __attribute__((packed)) dev_manager_t;

/**
 * @brief Runs a kernel device function
 * @param dev_id Device identifier.
 * @param fn_id Kernel device id
 * @param arg1 First argument
 * @param arg2 Second argument
 */
uint64_t dev_kernel_fn(uint64_t dev_id, uint64_t fn_id, uint64_t arg1, uint64_t arg2);

/**
 * @brief Registers a kernel callback function.
 * @param dev_id Device identifier.
 * @param fn_id Kernel function id
 * @param write Write function
 */
uint64_t dev_register_kernel_callback(uint64_t dev_id, uint64_t fn_id, kernel_fn write);

/**
 * @brief Retrieves the number of registered devices.
 * @param count [out] Pointer where device count will be stored.
 */
uint64_t dev_count();

/**
 * @brief Lists all current device IDs.
 * @param buffer [out] Pointer to a buffer where device IDs will be copied.
 * @param count Number of device IDs expected in the buffer.
 */
uint64_t dev_list(size_t buffer, size_t count);

/**
 * @brief Maps read-only metadata for a specific device into user-space.
 * @param dev_id Device identifier.
 * @param ptr [out] Pointer where mapped metadata address will be stored.
 */
uint64_t dev_info(uint64_t dev_id, size_t* ptr);

/**
 * @brief Opens a device and requests access.
 * @param dev_id Device identifier.
 */
uint64_t dev_open(uint64_t dev_id);

/**
 * @brief Closes access to a device and unmaps any shared memory.
 * @param dev_id Device identifier.
 */
uint64_t dev_close(uint64_t dev_id);

/**
 * @brief Creates a new device with the specified path and flags.
 * @param path Path to the new device.
 * @param flags Access and permission flags.
 */
uint64_t dev_create(const char* path, uint64_t flags);

/**
 * @brief Deletes a device permanently.
 * @param dev_id Device identifier.
 */
uint64_t dev_destroy(uint64_t dev_id);

/**
 * @brief Registers a callback function with a given signature.
 * @param dev_id Device identifier.
 * @param fn_id Function ID (used in dev_call).
 * @param signature Pointer to the callback signature.
 */
uint64_t dev_register_callback(uint64_t dev_id, uint64_t fn_id, dev_callback_signature* signature);

/**
 * @brief Unregisters a previously registered callback.
 * @param dev_id Device identifier.
 * @param fn_id Function ID to unregister.
 */
uint64_t dev_unregister_callback(uint64_t dev_id, uint64_t fn_id);

/**
 * @brief Maps a device's callback queue into user-space (only for trusted processes).
 * @param dev_id Device identifier.
 * @return queue address
 */
uint64_t dev_map_queue(uint64_t dev_id);

/**
 * @brief Unmaps a previously mapped callback queue.
 * @param dev_id Device identifier.
 */
uint64_t dev_unmap_queue(uint64_t dev_id);

/**
 * @brief Blocks the current process until a callback event is received.
 */
uint64_t dev_poll();

/**
 * @brief Waits for an event with a timeout (in milliseconds).
 * @param dev_id Device identifier.
 * @param timeout Maximum wait time.
 */
uint64_t dev_wait_event(uint64_t dev_id, uint64_t timeout);

/**
 * @brief Makes a device API call with up to 6 arguments.
 * @param dev_id Device identifier.
 * @param fn_id Function ID.
 * @param arg1-arg6 Arguments for the function.
 */
uint64_t dev_call(uint64_t dev_id, uint64_t fn_id, callback_args_t* args);

/**
 * @brief Shares user-allocated memory with a device.
 * @param dev_id Device identifier.
 * @param ptr Pointer to memory to share (must be 4KB-aligned).
 * @param size Size of memory (must be a multiple of 4KB).
 */
uint64_t dev_share(uint64_t dev_id, size_t ptr, size_t size);

/**
 * @brief Grants access to a device for all processes in a group.
 * @param dev_id Device identifier.
 * @param gid Group identifier.
 */
uint64_t dev_grant_access(uint64_t dev_id, uint64_t gid);

/**
 * @brief Grants trust to a group (enables queue mapping).
 * @param dev_id Device identifier.
 * @param gid Group identifier.
 */
uint64_t dev_grant_trust(uint64_t dev_id, uint64_t gid);

/**
 * @brief Revokes access to a device from a group.
 */
uint64_t dev_revoke_access(uint64_t dev_id, uint64_t gid);

/**
 * @brief Revokes trust from a group.
 */
uint64_t dev_revoke_trust(uint64_t dev_id, uint64_t gid);

/**
 * @brief Changes the device owner to another UID.
 */
uint64_t dev_set_owner(uint64_t dev_id, uint64_t uid);

/**
 * @brief Renames a device.
 */
uint64_t dev_rename(const char* old_path, const char* new_path);

#endif // DEVICE_H