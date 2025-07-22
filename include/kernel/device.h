/**
 * @file device.h
 * @brief High-level Device API for user-mode and kernel interaction.
 *
 * Provides a modern, secure, and high-performance abstraction layer for managing devices from user-space, including access control, memory sharing, callback registration, and device function invocation.
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
#define DEV_WRITE 0
#define DEV_READ 1
#define DEV_OPEN 2
#define DEV_CLOSE 3
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

#endif /* DEVICE_H */