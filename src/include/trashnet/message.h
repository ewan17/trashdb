#ifndef TBUFF_H
#define TBUFF_H

#define CANNOT_RESIZE -2

#define DEFAULT_TBUFF_SIZE 1024

typedef struct TBuffer {
    char *data;
    uint32_t start;
    uint32_t end;
    uint32_t capacity;
} TBuffer;

void init_tbuff(TBuffer *tBuff, uint32_t dispatchSize);
void kill_tbuffer(TBuffer *tBuff);

void add_byte_to_tbuffer(TBuffer *tBuff, const char data);
void add_bytes_to_tbuffer(TBuffer *tBuff, const char *data, uint32_t dataLen);

int get_byte_from_tbuffer(TBuffer *tBuff, int *byte, bool peek);

#define TBUFF_HAS_SPACE(tBuff) ((tBuff)->capacity - (tBuff)->end)
#define INIT_TBUFF(tBuff) (init_tbuff(tBuff, DEFAULT_TBUFF_SIZE))

#endif //TBUFF_H