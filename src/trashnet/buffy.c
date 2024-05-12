#include <arpa/inet.h>
#include <event2/bufferevent_ssl.h>

#include "trashnet.h"

enum CurrEv {
    READ_EV,
    WRITE_EV,
};

struct SendBuff {
    size_t endPtr;
    size_t startPtr;
    size_t sendBuffSize;

    char *sendBuff;

    pthread_mutex_t sbMutex;
};

/*
    @todo   SSL/TLS; use OpenSSL library 
    @todo   maybe add mdb txn or db env
*/ 
struct TrashData {
    int fd;
    enum CurrEv curr;
    struct event *ev;
    SSL *ctx;

    pthread_mutex_t tdMutex;

    char *dbname;
    char *username;

    size_t remainingMsg;
    Message *msg;
    bool partialMsg;

    SendBuff *sendBuff;

    size_t recvBuffLen;
    size_t recvBuffPtr;
    char recvBuff[RECV_BUFFY_SIZE];
};

static SendBuff *init_send_buff(size_t sendBuffSize);

static void close_client_internal(TrashData *td);
static void buffer_read_cb(evutil_socket_t fd, short flags, void *arg);
static void buffer_write_cb(evutil_socket_t fd, short flags, void *arg);

static int get_bytes_from_kernel(evutil_socket_t fd, char *buff, uint32_t buffLen, uint32_t *startPtr, uint32_t *endPtr);
static void get_bytes_from_buff(char *buff, uint32_t *startPtr, uint32_t *endPtr, char *strg, size_t strgLen);
static void put_bytes_on_buff(char *sendBuff, size_t *sendBuffLen, size_t *startPtr, size_t *endPtr, char *fromBuff, size_t fromBuffLen);
static int flush_data(evutil_socket_t fd, SendBuff *sb);
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
    td->sendBuff = init_send_buff(SEND_BUFFY_SIZE);

    td->msg = (struct Message *) malloc(sizeof(struct Message));
    if(td->msg == NULL) {
        goto error;
    }

    if(build_message(td->msg) != TRASH_SUCCESS) {
        goto error;
    }

    td->partialMsg = false;
    td->remainingMsg = 0;
    
    td->fd = clientFd;

    if(pthread_mutex_init(&td->tdMutex, NULL) != 0) {
        goto error;
    }

    /**
     * @note    will there be data in the recvBuffer on client connect?
     * @note    might need to handle the clients startup packet here
     * @todo    look into this
    */
    if(data_wait(td) != 0) {
        goto error;
    }

    return TRASH_SUCCESS;

error:
    evutil_closesocket(clientFd);
    free(ts);
    return TRASH_ERROR;
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
    pthread_mutex_lock(&td->tdMutex);
    if(event_del(td->ev) == -1) {
        printf("Error deleting read event\n");
        return TRASH_ERROR;
    }   

    if((td->ev = event_new(serverBase, td->fd, EV_WRITE, buffer_write_cb, (void *) td->sendBuff)) == NULL) {
        printf("Error creating new write event\n");
        return TRASH_ERROR;
    }

    if(event_add(td->ev, NULL)) {
        printf("Error creating new write event\n");
        return TRASH_ERROR;
    }

    td->curr = WRITE_EV;
    pthread_mutex_unlock(&td->tdMutex);

    return TRASH_SUCCESS;
}

bool trash_flush(TrashData *td) {
    if(td == NULL) {
        return false;
    }

    int rc;
    rc = flush_data(td->fd, td->sendBuff);

    if(rc < 0) {
        if(errno == EAGAIN) {
            if(data_send(td) != TRASH_SUCCESS) {
                /**
                 * @todo    kill the connection if data cannot be sent
                */
            }
        }
        return false;
    }

    return true;
}

/*
    @todo   needs fixing. not correct
*/ 
static void close_client_internal(TrashData *td) {
    if(td->fd != -1) {
        evutil_closesocket(td->fd);
    }
    event_del(td->ev);

    kill_message(td->msg);
    td->msg = NULL;

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

    Message *msg = td->msg;
    if(td->partialMsg) {
        if(td->remainingMsg < availBytes) {
            availBytes = td->remainingMsg;
        }

        td->remainingMsg -= availBytes;
    } else {
        uint32_t len;
        if(availBytes < TRASH_MSG_LEN) {
            goto get_more_data;
        }
        // get the length of the packet
        get_bytes_from_buff(td->recvBuff, &td->recvBuffPtr, &td->recvBuffLen, (char *) &len, 4);
        // the first 4 bytes represent the length of the packet
        len = ntohl(len);
        len -= 4;

        if(!msg_len_valid(len)) {
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

        td->remainingMsg = len - availBytes;
        td->partialMsg = (td->remainingMsg > 0);
    }

    if(!td->partialMsg) {
        // the enitre message fit within the recv buffer
        // no need to go get more data
        
    } else {
        add_bytes_to_message(msg, td->recvBuff + td->recvBuffPtr, availBytes);

        if(td->remainingMsg > 0) {
            goto get_more_data;
        } else {
            td->partialMsg = false;
        }
    }

    



    /**
     * @note    echo server for now
    */
    data_send(td);

    /**
     * @note    where we will send the pkt to the running threads to handle the parsing and the db work
    */
    // send message to threads
}

/**
 * @note    what do I do with the buffer after a successful write to the client?
*/
static void buffer_write_cb(evutil_socket_t fd, short flags, void *arg) {
    struct SendBuff *sb = (struct SendBuff *) arg;
    
    int rc = flush_data(fd, sb);
    if(rc != TRASH_SUCCESS) {
        /**
         * @todo    handle error
        */
    }

    /**
     * @note    set the send buffer size back to default size
     * @todo    am I always flushing all the data?
    */
    if(sb->sendBuffSize != SEND_BUFFY_SIZE) {
        pthread_mutex_lock(&sb->sbMutex);
        sb->sendBuffSize = SEND_BUFFY_SIZE;
        sb->sendBuff = trash_realloc(sb->sendBuff, SEND_BUFFY_SIZE);
        sb->startPtr = sb->endPtr = 0;
        pthread_mutex_unlock(&sb->sbMutex);
    }
};

static SendBuff *init_send_buff(size_t sendBuffSize) {
    SendBuff *buff;
    buff = (SendBuff *)malloc(sizeof(SendBuff *));
    if (buff == NULL) {
        return NULL;
    }

    buff->sendBuff = (char *) malloc(sendBuffSize * sizeof(char));
    if(buff->sendBuff == NULL) {
        free(buff);
        return NULL;
    }

    buff->sendBuffSize = sendBuffSize;
    buff->endPtr = buff->startPtr = 0;

    if(pthread_mutex_init(&buff->sbMutex, NULL) != 0) {
        free(buff->sendBuff);
        free(buff);
        return NULL;
    }

    return buff;
}


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
            return TRASH_EOF;
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
 * @todo    will need locking since we are moving data to the send buffer
 * @todo    this function needs fixing. We could be flushing data to the send buffer
 * @todo    resize the buffer if need be
*/
static void put_bytes_on_buff(char *sendBuff, size_t *sendBuffLen, size_t *startPtr, size_t *endPtr, char *fromBuff, size_t fromBuffLen) {
    size_t sendBuffSpace = *sendBuffLen - *endPtr;
    if(fromBuffLen > sendBuffSpace) {
        size_t needed = fromBuffLen - sendBuffSpace;
        resize_send_buff(&sendBuff, sendBuffLen, needed);
    }
    
    memmove(sendBuff + *endPtr, fromBuff, fromBuffLen);
    *endPtr += fromBuffLen;    
}

static int flush_data(evutil_socket_t fd, SendBuff *sb) {
    ssize_t len = 0;

    while(sb->startPtr < sb->endPtr) {
        ssize_t rc;
        rc = send(fd, sb->sendBuff, sb->endPtr - sb->startPtr, 0);
        if(rc == -1) {
            if(errno == EINTR) {
                continue;
            }
            return TRASH_ERROR;
        } else if(rc == 0) {
            return TRASH_EOF;
        }

        len += rc;

        pthread_mutex_lock(&sb->sbMutex);
        sb->startPtr += len;
        pthread_mutex_unlock(&sb->sbMutex);
    }

    pthread_mutex_lock(&sb->sbMutex);
    sb->startPtr = sb->endPtr = 0;
    pthread_mutex_unlock(&sb->sbMutex);

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