/**
 * @todo    generate IO thread and add to context struct
 * @todo    handle formatting later
 * 
 * @note    currently just dumping log messages to a file.
 * 
*/

#include <stdlib.h>
#include <pthread.h>
#include "logger.h"
#include "buffer.h"

typedef struct Context {
    pthread_t io_thread;
} Context;

typedef struct Logger {
    Context *context;
} Logger;

int init_logger(Logger **logger) {
    Context *new_context;
    new_context = (Context *)malloc(sizeof(Context));
    if (new_context == NULL) {
        return -1;
    }

    *logger = (Logger *)malloc(sizeof(Logger));
    if (*logger == NULL) {
        return -1;
    }

    Logger *new_logger = *logger;
    new_logger->context = new_context;
    return 0;
}

// void log(Logger *logger, Log_Level const * const log_level, char const * const message) {
//     // Buffer *buf;
//     // create_buffer(buf, MAX_LOG_BUFFER_SIZE);
       
//     // if(create_buffer(buf) == -1) {
//     //     /**
//     //      * @todo    Log malloc error and kill the process
//     //     */
//     // };
// }