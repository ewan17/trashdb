#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_PACKET_LEN 8192
#define MIN_PACKET_LEN 1

#define PACKET_LEN 4

#include <stdbool.h>

bool pkt_len_valid(uint32_t len);

#endif //PROTOCOL_H