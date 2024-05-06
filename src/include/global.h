#ifndef GLOBAL_H
#define GLOBAL_H

#include <pthread.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <stdio.h>
#include <signal.h>
#include <event2/event.h>

#include "server.h"
#include "utilities.h"
#include "message.h"
#include "protocol.h"
#include "buffy.h"

#define TRASH_SUCCESS (0)
#define TRASH_ERROR (-1)
#define TRASH_EOF (-2)

#define DB_SIZE 10485760

/**
 * @note    this needs fixing
*/
#define TRASH_PATH "../../../var/local/trashdb/"

#define LOG_DIR TRASH_PATH "logs/"

#define DB_DIR TRASH_PATH "data/"

/**
 * @note    this will change later
*/
#define MAX_DBS 1

extern struct event_base *serverBase;
extern pthread_mutex_t base_mutex;

typedef struct Settings {
    const char *path;
    char *port;
    
    size_t db_size;
    size_t num_dbs;
    size_t num_threads; 
} Settings;

#endif //GLOBAL_H
