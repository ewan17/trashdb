#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffer.h"

int check_buffer_empty(Buffer *buffer) {
    return (buffer->allocated_size == 0) ? 1 : 0;
}

int available_space(Buffer *buffer) {
    return buffer->allocated_size - strlen(buffer->data);
}

int change_buffer_size(char **dataBuff, size_t *size) {
    char *new_dataBuff;
    new_dataBuff = realloc(*dataBuff, *size);
    if(new_dataBuff == NULL) {
        free(*dataBuff);
        return -1;
    }

    *dataBuff = new_dataBuff;
    return 0;
}

int create_buffer(Buffer **buffer) {
    if (buffer == NULL) {
        return -1;
    }

    *buffer = (Buffer *)malloc(sizeof(Buffer));
    if (*buffer == NULL) {
        return -1;
    }

    Buffer *new_buffer = *buffer;  
    new_buffer->data = NULL,
    new_buffer->index = 0;
    new_buffer->allocated_size = 0;
    new_buffer->max_size = MAX_LOG_BUFFER_SIZE;
    return 0;
}

void append_data(Buffer *buffer, char const * const data) {
    if(check_buffer_empty(buffer) || buffer->data == NULL) {
        buffer->allocated_size = INITIAL_BUFFER_SIZE;
        buffer->data = (char *)malloc(INITIAL_BUFFER_SIZE*sizeof(char));
        if(buffer->data == NULL) {
            printf("Memory allocation error in test");
            /**
             * @todo    Handle differently
            */
        }
    }

    if(buffer->max_size > MAX_LOG_BUFFER_SIZE) {
        buffer->max_size = MAX_LOG_BUFFER_SIZE;
    }

    size_t i;
    size_t max_buffer_size = buffer->max_size;
    size_t new_buffer_size = buffer->allocated_size;
    size_t current_buff_index = buffer->index;
    for(i = 0; i < strlen(data); i++) {
        if(current_buff_index == (max_buffer_size - 1)) {
            break;    
        }

        // realloc size when necessary
        if(current_buff_index == (new_buffer_size - 1)) {
            new_buffer_size = new_buffer_size * 2;
            if(change_buffer_size(&buffer->data, &new_buffer_size) != 0) {
                /**
                 * @todo    error allocating memory
                */
            };
        }
        
        buffer->data[current_buff_index] = data[i];
        current_buff_index++;
    }

    buffer->data[current_buff_index] = '\0';
    buffer->index = current_buff_index;
    buffer->allocated_size = new_buffer_size;
}

/**
 * @todo
*/
void flush_data(Buffer *buffer) {
    // push data to the io thread
    kill_buffer(buffer);
}

/**
 * @todo
*/
void reset_buffer(Buffer *buffer) {
    free(buffer->data);
    buffer->data = NULL;
    buffer->index = 0;
    buffer->allocated_size = 0;
    buffer->max_size = MAX_LOG_BUFFER_SIZE;
}

void kill_buffer(Buffer *buffer) {
    if(buffer->data != NULL) {
        free(buffer->data);
    }
    free(buffer);
    buffer = NULL;
}