#include <fs/stream.h>

#include <memory/kmemory.h>

/**
 * @brief Creates a single stream
 * @return Returns a pointer to the stream
 * 
 * A single stream is a stream that is one directional
 */
single_stream_buffer_t* single_stream_create()
{
    /* Allocate and return stream */
    single_stream_buffer_t* stream_buffer = kmalloc(sizeof(single_stream_buffer_t));
    stream_buffer->stream.head = 0;
    stream_buffer->stream.tail = 0;
    

    return stream_buffer;
}

/**
 * @brief Creates a dual stream
 * @return Returns a pointer to the stream
 * 
 * A dual stream is a bidirectional stream meaning
 * it can be read from and written into on both sides
 * This stream also has two buffers, one for each direction
 * of travel
 */
dual_stream_buffer_t* dual_stream_create()
{
    /* Allocate and return stream */
    dual_stream_buffer_t* stream_buffer = kmalloc(sizeof(dual_stream_buffer_t));
    stream_buffer->stream_in.head = 0;
    stream_buffer->stream_in.tail = 0;
    stream_buffer->stream_out.head = 0;
    stream_buffer->stream_out.tail = 0;

    return stream_buffer;
}

/**
 * @brief Creates a hybrid stream
 * @return Returns a poitner to the stream
 * 
 * A hybrid stream is a bidirectional stream meaning
 * it can be read from and written into on both sides
 * This stream also has only one buffer, so the data
 * going each direction shares the buffer
 */
hybrid_stream_buffer_t* hybrid_stream_create()
{
    /* Allocate and return stream */
    hybrid_stream_buffer_t* stream_buffer = kmalloc(sizeof(hybrid_stream_buffer_t));
    stream_buffer->stream_in.head = 0;
    stream_buffer->stream_in.tail = 0;
    stream_buffer->stream_out.head = 0;
    stream_buffer->stream_out.tail = 0;

    return stream_buffer;
}

/**
 * @brief Writes to a stream
 * @param endpoint Stream end point being writing to
 * @param buffer Buffer being read from
 * @param len Maximum number of bytes being written
 */
void stream_write(stream_endpoint_t* endpoint, 
                          uint8_t* buffer, size_t len)
{
    /* Fetch data */
    uint8_t* data = endpoint->write_data;
    size_t* head = &endpoint->write_stream->head;
    size_t* tail = &endpoint->write_stream->tail;
    
    size_t first_chunk = STREAM_SIZE - *head;

    if (len <= first_chunk)
    {
        kmemcpy(&buffer[*head], data, len);
        *head = (*head + len) % STREAM_SIZE;
    }
    else
    {
        kmemcpy(&buffer[*head], data, first_chunk);
        kmemcpy(buffer, data + first_chunk, len - first_chunk);
        *head = (len - first_chunk);
    }

    /* handle overwrite of tail */
    size_t dist = (*head >= *tail) ? (*head - *tail) : (STREAM_SIZE - *tail + *head);
    if (dist >= STREAM_SIZE)
    {
        *tail = (*head + 1) % STREAM_SIZE;
    }

}

/**
 * @brief Reads from a stream
 * @param endpoint Stream end point being read from
 * @param tail The current position of the file descriptor
 * @param buffer Buffer being written to
 * @param len Maximum number of bytes read
 */
size_t stream_read(stream_endpoint_t* endpoint, 
                         size_t* tail, uint8_t* buffer, size_t len)
{
    /* Fetch data */
    uint8_t* data = endpoint->read_data;
    size_t* head = &endpoint->write_stream->head;

    size_t available = (*head >= *tail)
        ? (*head - *tail)
        : (STREAM_SIZE - *tail + *head);

    size_t to_read = (len < available) ? len : available;

    size_t first_chunk = STREAM_SIZE - *tail;
    if (to_read <= first_chunk)
    {
        kmemcpy(buffer, &data[*tail], to_read);
        *tail = (*tail + to_read) % STREAM_SIZE;
    }
    else
    {
        kmemcpy(buffer, &data[*tail], first_chunk);
        kmemcpy(buffer + first_chunk, data, to_read - first_chunk);
        *tail = to_read - first_chunk;
    }

    return to_read;
}

/**
 * @brief Returns an
 */
size_t* stream_access(stream_endpoint_t* endpoint)
{
    
}












/* Call the read function pointer of the input stream */
long istream_read(istream_t *stream, void *buffer, unsigned long size)
{
    if (!stream || !stream->read)
    return -1;  /* error or unsupported operation */
    
    return stream->read(stream->ctx, buffer, size);
}

long istream_select(istream_t *stream)
{

}

long istream_poll(istream_t *stream)
{

}

long istream_epoll(istream_t *stream)
{

}

/* Call the write function pointer of the output stream */
long ostream_write(ostream_t *stream, const void *buffer, unsigned long size)
{
    if (!stream || !stream->write)
        return -1;  /* error or unsupported operation */
    
    return stream->write(stream->ctx, buffer, size);
}

long ostream_select(istream_t *stream)
{

}

long ostream_poll(istream_t *stream)
{

}

long ostream_epoll(istream_t *stream)
{
    
}