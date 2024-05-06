#ifndef BUFFY_H
#define BUFFY_H

#include <stdbool.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <event2/util.h>

#include "utilities.h"
#include "message.h"

#define SOCKET_READ_TIMEOUT 10
#define SOCKET_WRITE_TIMEOUT 10

#define RECV_BUFFY_SIZE 4096
#define SEND_BUFFY_SIZE 4096

enum CurrEv{
    READ_EV,
    WRITE_EV,
};

/*
    @todo   SSL/TLS; use OpenSSL library 
    @todo   maybe add mdb txn or db env
*/ 
typedef struct TrashData {
    int fd;
    enum CurrEv curr;
    struct event *ev;
    SSL *ctx;

    char *dbname;
    char *username;

    size_t remainingPacket;
    Message *packet;
    bool partialPacket;

    size_t sendBuffLen;
    size_t sendBuffPtr;
    size_t sendBuffSize;
    char *sendBuff;

    size_t recvBuffLen;
    size_t recvBuffPtr;
    char recvBuff[RECV_BUFFY_SIZE];

} TrashData;

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

#define RECV_SPACE(td) (RECV_BUFFY_SIZE - (td)->recvBuffLen)
#define BYTES_RECV(td) ((td)->recvBuffLen - (td)->recvBuffPtr)
#define SEND_SPACE(td) ((td)->sendBuffSize - (td)->sendBuffLen)

#endif //BUFFY_H