#ifndef MESSAGE_H
#define MESSAGE_H

#define CANNOT_RESIZE -2

#define DEFAULT_DISPATCH_SIZE 1024

typedef struct Message {
    char *data;
    uint32_t start;
    uint32_t end;
    uint32_t capacity;
} Message;

int build_message(Message *message);
void dispatch_message(SendBuff *sendBuff);
void kill_message(Message *message);

void add_byte_to_message(Message *message, const char data);
void add_bytes_to_message(Message *message, const char *data, uint32_t dataLen);

int get_byte_from_message(Message *message, int *byte, bool peek);

#define MESSAGE_SPACE(msg) ((msg)->capacity - (msg)->end)

#endif //MESSAGE_H