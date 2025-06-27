/* stream.h */
#ifndef STREAM_H
#define STREAM_H

#include <kernel/process.h>
#include <kint.h>

#define STREAM_SIZE 4096

#define STREAM_READ 1
#define STREAM_WRITE 2

typedef struct stream_t
{
    size_t head;
    size_t tail;
    uint8_t flags;
} stream_t;

typedef struct single_stream_buffer_t
{
    uint8_t data[STREAM_SIZE];
    stream_t stream;
} __attribute__((packed)) single_stream_buffer_t;

typedef struct dual_stream_buffer_t
{
    uint8_t data_in[STREAM_SIZE];
    uint8_t data_out[STREAM_SIZE];
    stream_t stream_in;
    stream_t stream_out;
} __attribute__((packed)) dual_stream_buffer_t;

typedef struct hybrid_stream_buffer_t
{
    uint8_t data[STREAM_SIZE];
    stream_t stream_in;
    stream_t stream_out;
} __attribute__((packed)) hybrid_stream_buffer_t;

typedef struct stream_endpoint_t
{
    uint8_t* read_data;
    uint8_t* write_data;
    stream_t* read_stream;
    stream_t* write_stream;
} stream_endpoint_t;

single_stream_buffer_t* single_stream_create();
dual_stream_buffer_t* dual_stream_create();
hybrid_stream_buffer_t* hybrid_stream_create();
void stream_write(stream_endpoint_t* endpoint, uint8_t* buffer, size_t len);
size_t stream_read(stream_endpoint_t* endpoint, size_t* tail, uint8_t* buffer, size_t len);

typedef struct istream_t istream_t;
typedef struct ostream_t ostream_t;

/* Function pointer typedefs for stream operations */
typedef long (*istream_read_fn)(void* ctx, void* buffer, long size);
typedef long (*ostream_write_fn)(void* ctx, void* buffer, long size);

/* Input stream struct */
struct istream_t
{
    void* ctx;            /* user-defined context (file handle, buffer ptr, etc) */
    istream_read_fn read; /* backend read function pointer */
};

/* Output stream struct */
struct ostream_t
{
    void* ctx;              /* user-defined context */
    ostream_write_fn write; /* backend write function pointer */
};

/* stream data */

/* Generic read/write wrappers that call the backend functions */
long istream_read(istream_t* stream, void* buffer, unsigned long size);
long ostream_write(ostream_t* stream, const void* buffer, unsigned long size);

#endif /* STREAM_H */