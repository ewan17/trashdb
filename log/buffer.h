#define INITIAL_BUFFER_SIZE 2
#define MAX_LOG_BUFFER_SIZE 512

typedef struct Buffer {
    char *data;
    size_t index;
    size_t allocated_size;
    size_t max_size;
} Buffer;

/**
 * Creates a buffer with an initial size of 64.
 * 
 * @param   buffer  struct that contains the data and the size
 * 
 * @return  0 on success or -1 on memory allocation error
*/
int create_buffer(Buffer **buffer);

/**
 * Append data to the buffer.
 * 
 * @param   buffer  struct that contains the data and the size
 * @param   data    data that will be appened to the buffer 
 * 
*/
void append_data(Buffer *buffer, char const * const data);

/**
 * 
*/
void flush_data(Buffer *buffer);

/**
 * Resets the buffer
 * 
 * @param   buffer  pointer to a buffer
*/
void reset_buffer(Buffer *buffer);

/**
 * Kills the buffer
 * 
 * @param   buffer  pointer to a buffer
*/
void kill_buffer(Buffer *buffer);