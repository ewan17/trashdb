#ifndef BUFFY_H
#define BUFFY_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <event2/util.h>

#include "trashnet.h"

#define SOCKET_READ_TIMEOUT 10
#define SOCKET_WRITE_TIMEOUT 10

#define RECV_BUFFY_SIZE 4096
#define SEND_BUFFY_SIZE 4096

typedef struct SendBuff SendBuff;
typedef struct TrashData TrashData;

/* add state to the socket later */
typedef struct TrashSocket {
    struct TrashAddr remoteAddr;
    struct TrashAddr localAddr;

    TrashData *td;
} TrashSocket;

int construct_client(evutil_socket_t clientFd, TrashData **td, bool isUnixSock);
int close_client(TrashData *td);

int data_wait(TrashData *td);
int data_send(TrashData *td);

bool trash_flush(TrashData *td);

/**
 * @note    this should be revised
 */
#define LOCK(sb) (pthread_mutex_lock(&sb->sbMutex))
#define UNLOCK(sb) (pthread_mutex_unlock(&sb->sbMutex))

#define RECV_SPACE(td) (RECV_BUFFY_SIZE - (td)->recvBuffLen)
#define BYTES_RECV(td) ((td)->recvBuffLen - (td)->recvBuffPtr)
#define SEND_SPACE(td) ((td)->sendBuffSize - (td)->sendBuffLen)

#endif //BUFFY_H