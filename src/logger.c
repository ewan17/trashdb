#include "global.h"

#ifdef TEST
#define LOG_DIR "./test/logs/"
#else
#define LOG_DIR VAR_PATH "logs/"
#endif

#define CLOSE_Q 0X01
#define OPEN_Q 0x02

// this can change if more levels are added
#define MAX_LEVEL_LEN 5

#define TIME_LENGTH 26
#define LOG_FMT "[%s][%s][%s:%d][%s]\n"

struct LogInfo {
    IL move;

    const char *message;
    const char *file;
    int line;

    LEVEL level;
};

static LL logs;

static FILE *f;
static pthread_t logThread;

static pthread_mutex_t logMutex;
static pthread_cond_t logCond;
static unsigned int flags = 0;

/** ---- INTERNAL FUNCTIONS ---- **/
static void level_to_char(char *buff, LEVEL level);
static void current_time(char *timestamp);
static void *log_func();

/**
 * Open the log file and start the logging thread.
 */
int init_logger() {
    char filepath[256];
    int rc, attempts = 0;

    init_list(&logs);

    pthread_mutex_init(&logMutex, NULL);
    pthread_cond_init(&logCond, NULL);

    // read/write file
    snprintf(filepath, sizeof(filepath), "%s%s", LOG_DIR, logFile);

    do {
        attempts++;

        f = fopen(filepath, "a+");
        if(f == NULL) {
            if(errno == ENOENT) {
                rc = trash_no_file_dir(filepath);
                // this should always return true
                assert(rc == 0);
            }

            if(errno == EACCES) {
                // handle this case later
                assert(0);
            }
        }
    } while (f == NULL && attempts < 2);

    if(f == NULL) {
        return TRASH_ERROR;
    }

    flags |= OPEN_Q;

    rc = pthread_create(&logThread, NULL, log_func, NULL);
    return rc;
}

/**
 * Stops accepting logs.
 * Finishes flushing remaining logs.
 */
void close_logger() {
    int rc;

    pthread_mutex_lock(&logMutex);
    // close the log q
    flags = CLOSE_Q;
    // finish processing the logs
    pthread_cond_signal(&logCond);
    pthread_mutex_unlock(&logMutex);

    rc = pthread_join(logThread, NULL);
    assert(rc == 0);

    pthread_mutex_destroy(&logMutex);
    pthread_cond_destroy(&logCond);
}

/**
 * Writes the log message to the log file.
 * 
 * @param   file        file name
 * @param   line        line where func was called
 * @param   message     log message
 * @param   LEVEL       log level
 */
int trash_log(LEVEL level, const char *file, int line, const char *message) {
    struct LogInfo *log;

    log = (struct LogInfo *)malloc(sizeof(*log));
    assert(log != NULL);

    init_il(&log->move);

    log->message = message;
    log->file = file;
    log->line = line;
    log->level = level;

    pthread_mutex_lock(&logMutex);
    // check if the log q is open
    if(!(flags & OPEN_Q)) {
        /**
         * @note    this will throw an assert error currently
         * @todo    fix later
         */
        pthread_mutex_unlock(&logMutex);    
        return LOG_ERROR;
    }
    // finish processing the logs
    list_append(&logs, &log->move);
    // if it is 1 then it just came from 0 which was probably an idle thread
    if(logs.len < 1) {
        pthread_cond_signal(&logCond);
    }
    pthread_mutex_unlock(&logMutex);

    return LOGGED;
}

/**
 * Converts the log level enums to their char representation.
 * 
 * @param   buff    buffer for the level char
 * @param   level   log level
 */
static void level_to_char(char *buff, LEVEL level) {
    switch (level) {
        case INFO:
            strcpy(buff, "INFO");
            break;
        case DEBUG:
            strcpy(buff, "DEBUG");
            break;
        case WARN:
            strcpy(buff, "WARN");
            break;
        case ERROR:
            strcpy(buff, "ERROR");
            break;
    }
}

/**
 * Creates a Local Date Time.
 * Example: Mon Jan 01 23:59:59 2024
 * 
 * @param   timestamp        address where the timestamp should be stored
 */
static void current_time(char *timestamp) {
    time_t now;
    struct tm *t;
    
    now = time(NULL);
    t = localtime(&now);

    char *asct = asctime(t);
    memcpy(timestamp, asct, TIME_LENGTH - 2);
}

/**
 * Log function that is executed by the logging thread.
 * 
 * @note    Currently, data is flushed immediately, if the the logs begin to build up,
 * @note    we may need to add an interval for when to call flush.
 * 
 */
static void *log_func() {
    struct LogInfo *log;
    IL *item;
    // do not include the '\n'
    char timestamp[TIME_LENGTH - 2];
    // +1 for null terminated byte
    char levelBuff[MAX_LEVEL_LEN + 1];
    
    while(1) {
        pthread_mutex_lock(&logMutex);
        while(logs.len == 0 && flags & OPEN_Q) {
            pthread_cond_wait(&logCond, &logMutex);
        }

        if(logs.len == 0 && flags & CLOSE_Q) {
            break;
        }

        // grab the log struct
        item = list_pop(&logs);
        log = CONTAINER_OF(item, struct LogInfo, move);

        // set the time and print to the file
        level_to_char(levelBuff, log->level);
        current_time(timestamp);
        fprintf(f, LOG_FMT, timestamp, levelBuff, log->file, log->line, log->message);
        // for now we will flush after every write
        // if this is slow we can flush on intervals maybe
        fflush(f);
        free(log);
        pthread_mutex_unlock(&logMutex);
    }

    return NULL;
} 