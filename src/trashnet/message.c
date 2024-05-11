#include <limits.h>

#include "trashnet.h"

static int init_message(Message *message, uint32_t dispatchSize);
static void reset_message(Message *message);
static void resize_message(Message *message, uint32_t size);

int build_message(Message *message) {
    if(message == NULL) {
        return TRASH_ERROR;
    }

    int rc;
    rc = init_message(message, DEFAULT_DISPATCH_SIZE);
    return rc;
}

void dispatch_message(SendBuff *sendBuff) {
    
}

void kill_message(Message *message) {
    if(message != NULL) {
        free(message->data);
        free(message);
    }
}

/**
 * @todo    fix this function
*/
void dispatch_message(Message *message) {
    // move data to send buff
    // create event

    free(message->data);
    message->data = NULL;
}

void add_byte_to_message(Message *message, const char data) {
    if(message == NULL) {
        return;
    }

    resize_message(message, 1);

    message->data[message->end] = data;
    message->end++;

    message->data[message->end] = '\0';
}

void add_bytes_to_message(Message *message, const char *data, uint32_t dataLen) {
    if(message == NULL) {
        return;
    }

    resize_message(message, dataLen);

    memcpy(message->data + message->end, data, dataLen);
    message->end += dataLen;

    message->data[message->end] = '\0';
}

int get_byte_from_message(Message *message, int *byte, bool peek) {
    if(message == NULL || (message->start == message->end)) {
        return TRASH_ERROR;
    }

    uint32_t startPtr = message->start;
    if(!peek) {
        message->start++;
    }

    *byte = (unsigned char) message->data[startPtr];

    return TRASH_SUCCESS;
}

static void reset_message(Message *message) {
    message->start = message->end = 0;
}

static int init_message(Message *message, uint32_t dispatchSize) {
    reset_message(message);

    if(dispatchSize > DEFAULT_DISPATCH_SIZE) {
        dispatchSize = DEFAULT_DISPATCH_SIZE;
    }
    message->data = (char *) malloc(sizeof(char) * dispatchSize);
    if(message->data == NULL) {
        return TRASH_ERROR;
    }

    message->capacity = dispatchSize;
    return TRASH_SUCCESS;
}

/**
 * @note    this function may need to realloc to a larger size
*/
static void resize_message(Message *message, uint32_t size) {
    if(message->end >= (UINT32_MAX - size)) {
        goto error;
    }

    // +1 for the null byte
    uint16_t availSpace = message->capacity - (message->end + 1); 

    if(size > availSpace) {
        if(message->capacity == UINT32_MAX) {
            goto error;
        }

        size_t needed = message->capacity * 2;
        while(needed < size) {
            needed = needed * 2;
        }

        if(needed > UINT32_MAX) {
            needed = UINT32_MAX;
        }

        message->data = trash_realloc(message->data, needed);
        if(message->data == NULL) {
            goto error;
        }
        message->capacity = needed;
    }

    return;

error:
    /**
     * @todo    add some sort of error handling here for invalid realloc or for a resize limit
     * @note    the code below should be changed
    */
    printf("error resizing message\n");
    exit(1);
}
