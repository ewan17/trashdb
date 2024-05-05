#include <string.h>
#include <stddef.h>
#include <arpa/inet.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/bufferevent_ssl.h>

#include "../trash/global.h"
#include "protocol.h"
#include "utilities.h"
#include "buffy.h"

static void close_client_internal(TrashData *td);
static void buffer_read_cb(evutil_socket_t sock, short flags, void *arg);
static void buffer_write_cb(evutil_socket_t sock, short flags, void *arg);

static int get_bytes_from_kernel(evutil_socket_t fd, char *buff, uint32_t buffLen, uint32_t *startPtr, uint32_t *endPtr);
static void get_bytes_from_buff(char *buff, uint32_t *startPtr, uint32_t *endPtr, char *strg, size_t strgLen);
static void put_bytes_on_buff(evutil_socket_t fd, char *sendBuff, size_t *sendBuffLen, size_t *startPtr, size_t *endPtr, char *fromBuff, size_t fromBuffLen);
static int flush_data(evutil_socket_t fd, char *buff, size_t buffLen, size_t *startPtr, size_t *endPtr);
static void resize_send_buff(char **sendBuff, size_t *sendBuffLen, size_t needed);

/**
    @todo   states for socket need to be added later
    @todo   to construct the client, we will need the startup packet
    @todo   process the startup packet
*/
int construct_client(evutil_socket_t clientFd, TrashData **ts, bool isUnixSock) {
    *ts = (TrashData *) malloc(sizeof(TrashData));
    if(*ts == NULL) {
        goto error;
    }

    TrashData *td = *ts;

    if(!isUnixSock) {
        /* configure ssl */
        perror("need to configure ssl");
        goto error;
    }

    td->recvBuffPtr = td->recvBuffLen = 0;
    td->sendBuffPtr = td->sendBuffLen = 0;

    td->sendBuff = (char *) malloc(SEND_BUFFY_SIZE * sizeof(char));
    if(td->sendBuff == NULL) {
        goto error;
    }

    td->sendBuffSize = SEND_BUFFY_SIZE;

    td->packet = (struct Message *) malloc(sizeof(struct Message));
    if(td->packet == NULL) {
        goto error;
    }

    if(build_message(td->packet) != TRASH_SUCCESS) {
        goto error;
    }

    td->partialPacket = false;
    td->remainingPacket = 0;
    
    td->fd = clientFd;
    if(data_wait(td) != 0) {
        goto error;
    }

    return TRASH_SUCCESS;

error:
    evutil_closesocket(clientFd);
    free(ts);
    return TRASH_ERROR;
} 

int data_wait(TrashData *td) { 
    if((td->ev = event_new(serverBase, td->fd, EV_READ|EV_PERSIST, buffer_read_cb, (void *) td)) == NULL) {
        printf("Error creating new wait event\n");
        return TRASH_ERROR;
    }

    if(event_add(td->ev, NULL) != 0) {
        printf("Error creating wait event\n");
        return TRASH_ERROR;
    }

    td->curr = READ_EV;
    return TRASH_SUCCESS;
}

int data_send(TrashData *td) { 
    if(event_del(td->ev) == -1) {
        printf("Error deleting read event\n");
        return TRASH_ERROR;
    }   

    if((td->ev = event_new(serverBase, td->fd, EV_WRITE, buffer_write_cb, (void *) td)) == NULL) {
        printf("Error creating new write event\n");
        return TRASH_ERROR;
    }

    if(event_add(td->ev, NULL)) {
        printf("Error creating new write event\n");
        return TRASH_ERROR;
    }

    td->curr = WRITE_EV;

    return TRASH_SUCCESS;
}

/**
 * @todo    needs fixing
*/
int close_client(TrashData *td) {
    if(td == NULL) {
        return TRASH_ERROR;
    }
    close_client_internal(td);
    return TRASH_SUCCESS;
}

/*
    @todo   needs fixing. not correct
*/ 
static void close_client_internal(TrashData *td) {
    if(td->fd != -1) {
        evutil_closesocket(td->fd);
    }
    event_del(td->ev);

    kill_message(td->packet);
    td->packet = NULL;

    free(td->sendBuff);
    td->sendBuff = NULL;
}

/**
 * @todo    do not spend too long on one client
 * @todo    fix function. currently works as echo server.
 * @todo    handle errors for getting kernel data
 * @todo    add some queue so the write events can grab the desired writes
 * @todo    pass the buffy and the read data to thread pool to handle work which will call send_data when ready.
 * @todo    add some sort of parser that can parse the buffer and send the data off to the thread pool.
*/
static void buffer_read_cb(evutil_socket_t sock, short flags, void *arg) {
    TrashData *td = (TrashData *) arg;
    uint32_t availBytes;

get_more_data:
    int rc;
    if(RECV_SPACE(td) > 0) {
        rc = get_bytes_from_kernel(td->fd, td->recvBuff, sizeof(td->recvBuff), &td->recvBuffPtr, &td->recvBuffLen);

        if(rc == EOF) {
            /**
             * @note    needs fixing
            */
            close_client_internal(td);
            return;
        }

        if(rc == TRASH_ERROR) {
            // would of blocked
            return;
        }

        availBytes = BYTES_RECV(td);
        if(availBytes <= 0) {
            goto get_more_data;
        }
    }

    Message *pkt = td->packet;
    if(td->partialPacket) {
        if(td->remainingPacket < availBytes) {
            availBytes = td->remainingPacket;
        }

        td->remainingPacket -= availBytes;
    } else {
        uint32_t len;
        if(availBytes < PACKET_LEN) {
            goto get_more_data;
        }
        // get the length of the packet
        get_bytes_from_buff(td->recvBuff, &td->recvBuffPtr, &td->recvBuffLen, (char *) &len, 4);
        // the first 4 bytes represent the length of the packet
        len = ntohl(len);
        len -= 4;

        if(!pkt_len_valid(len)) {
            /**
             * @note    needs fixing
            */
            // disconnect connection since we cannot determine where the packet really begins and ends
            close_client_internal(td);
            return;
        }

        if(len < availBytes) {
            availBytes = len;
        }

        td->remainingPacket = len - availBytes;
    }

    add_bytes_to_message(pkt, td->recvBuff + td->recvBuffPtr, availBytes);

    if(td->remainingPacket > 0) {
        td->partialPacket = true;
        goto get_more_data;
    }

    /**
     * @note    where we will send the pkt to the running threads to handle the parsing and the db work
    */
    // send message to threads
}

// what do I do with the buffer after a successful write to the client?
static void buffer_write_cb(evutil_socket_t sock, short flags, void *arg) {
    TrashData *td = (TrashData *) arg;
    
    int rc = flush_data(td->fd, td->sendBuff, td->sendBuffSize, &td->sendBuffPtr, &td->sendBuffLen);
    if(rc != TRASH_SUCCESS) {
        // handle error
    }

    td->sendBuffSize = SEND_BUFFY_SIZE;
    
};

/* ---------- SEND & RECV FUNCTIONALITY ---------- */

/**
 * Helper function to retrieve bytes from the kernel and add them to the recv buffer
*/
static int get_bytes_from_kernel(evutil_socket_t fd, char *buff, uint32_t buffLen, uint32_t *startPtr, uint32_t *endPtr) {
    ssize_t len;
    
    if(*startPtr >= *endPtr) {
        *startPtr = *endPtr = 0;
    }

    if(*startPtr > 0) {
        size_t untouched = *endPtr - *startPtr;
        memmove(buff, buff + *startPtr, untouched);
        *startPtr = 0;
        *endPtr -= untouched;  
    }

    for(;;) {
        len = recv(fd, buff, buffLen - *endPtr, 0);
        if(len == -1) {
            if(errno == EINTR) {
                continue;
            }
            return TRASH_ERROR;
        } else if(len == 0) {
            return EOF;
        }
        *endPtr += len;
        return TRASH_SUCCESS;
    }

}

/**
 * Helper function to retrieve bytes from the recv buffer
*/
static void get_bytes_from_buff(char *buff, uint32_t *startPtr, uint32_t *endPtr, char *strg, size_t strgLen) {
    uint16_t amnt;
    amnt = *endPtr - *startPtr;
    if(strgLen < amnt) {
        amnt = strgLen;
    }

    memcpy(strg, buff + *startPtr, amnt);
    *startPtr += amnt;
}

/**
 * @todo    this function needs fixing. We could be flushing data to the send buffer
 * @todo    resize the buffer if need be
*/
static void put_bytes_on_buff(evutil_socket_t fd, char *sendBuff, size_t *sendBuffLen, size_t *startPtr, size_t *endPtr, char *fromBuff, size_t fromBuffLen) {
    size_t sendBuffSpace = *sendBuffLen - *endPtr;
    if(fromBuffLen > sendBuffSpace) {
        size_t needed = fromBuffLen - sendBuffSpace;
        resize_send_buff(&sendBuff, sendBuffLen, needed);
    }
    
    memmove(sendBuff + *endPtr, fromBuff, fromBuffLen);
    *endPtr += fromBuffLen;    
}

static int flush_data(evutil_socket_t fd, char *buff, size_t buffLen, size_t *startPtr, size_t *endPtr) {
    ssize_t len = 0;

    while(*startPtr < *endPtr) {
        ssize_t rc;
        rc = send(fd, buff, *endPtr - *startPtr, 0);
        if(rc == -1) {
            if(errno == EINTR) {
                continue;
            }
            return TRASH_ERROR;
        } else if(rc == 0) {
            return EOF;
        }

        len += rc;
        *startPtr += len;
    }

    *startPtr = *endPtr = 0;
    
    return TRASH_SUCCESS;
}

static void resize_send_buff(char **sendBuff, size_t *sendBuffLen, size_t needed) {
    if(needed > *sendBuffLen) {
        char *newSendBuff = (char *) trash_realloc(*sendBuff, needed);

        if(newSendBuff == NULL) {
            /**
             * @todo    throw an error
            */
        }

        *sendBuff = newSendBuff;
        *sendBuffLen = needed;
    }
}