#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_PACKET_LEN 8192
#define MIN_PACKET_LEN 1

// first 4 bytes are the message length
#define TRASH_MSG_LEN 4

bool msg_len_valid(uint32_t len);

#endif //PROTOCOL_H