#include <limits.h>

#include "trashnet.h"

static void reset_tbuff(TBuffer *tBuff);
static void resize_tbuff(TBuffer *tBuff, uint32_t size);

void init_tbuff(TBuffer *tBuff, uint32_t size) {
    reset_tbuff(tBuff);

    if(size > DEFAULT_TBUFF_SIZE) {
        size = DEFAULT_TBUFF_SIZE;
    }
    tBuff->data = (char *) malloc(sizeof(char) * size);
    assert(tBuff->data != NULL);

    tBuff->capacity = size;
}

void kill_tbuffer(TBuffer *tBuff) {
    if(tBuff != NULL) {
        free(tBuff->data);
        free(tBuff);
    }
}

void add_byte_to_tbuffer(TBuffer *tBuff, const char data) {
    if(tBuff == NULL) {
        return;
    }

    resize_tbuff(tBuff, 1);

    tBuff->data[tBuff->end] = data;
    tBuff->end++;

    tBuff->data[tBuff->end] = '\0';
}

void add_bytes_to_tbuffer(TBuffer *tBuff, const char *data, uint32_t dataLen) {
    if(tBuff == NULL) {
        return;
    }

    resize_tbuff(tBuff, dataLen);

    memcpy(tBuff->data + tBuff->end, data, dataLen);
    tBuff->end += dataLen;

    tBuff->data[tBuff->end] = '\0';
}

int get_byte_from_tbuffer(TBuffer *tBuff, int *byte, bool peek) {
    if(tBuff == NULL || (tBuff->start == tBuff->end)) {
        return TRASH_ERROR;
    }

    uint32_t startPtr = tBuff->start;
    if(!peek) {
        tBuff->start++;
    }

    *byte = (unsigned char) tBuff->data[startPtr];

    return TRASH_SUCCESS;
}

static void reset_tbuff(TBuffer *tBuff) {
    tBuff->start = tBuff->end = 0;
}

/**
 * @note    this function may need to realloc to a larger size
*/
static void resize_tbuff(TBuffer *tBuff, uint32_t size) {
    if(tBuff->end >= (UINT32_MAX - size)) {
        goto error;
    }

    // +1 for the null byte
    uint16_t availSpace = tBuff->capacity - (tBuff->end + 1); 

    if(size > availSpace) {
        if(tBuff->capacity == UINT32_MAX) {
            goto error;
        }

        size_t needed = tBuff->capacity * 2;
        while(needed < size) {
            needed = needed * 2;
        }

        if(needed > UINT32_MAX) {
            needed = UINT32_MAX;
        }

        tBuff->data = trash_realloc(tBuff->data, needed);
        if(tBuff->data == NULL) {
            goto error;
        }
        tBuff->capacity = needed;
    }

    return;

error:
    /**
     * @todo    add some sort of error handling here for invalid realloc or for a resize limit
     * @note    the code below should be changed
    */
    assert(0);
}
