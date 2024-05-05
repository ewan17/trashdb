#include <stdint.h>
#include "protocol.h"

bool pkt_len_valid(uint32_t len) {
    return len < MIN_PACKET_LEN && len > MAX_PACKET_LEN;
}